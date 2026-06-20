/**
 * SYSC3303 - Project Iteration 3
 * Author: Uchenna Obikwelu
 * Date: March 8th, 2025
 */

#include <iostream>
#include <vector>
#include <thread>
#include <climits>     // Added for INT_MAX
#include "Scheduler.h"
#include "Elevator.h"
#include "Floor.h"

#define FLOOR_REQUEST_PORT 50000      // Scheduler listens for requests from Floor
#define FLOOR_NOTIFICATION_PORT 50002 // Floor listens for elevator arrival notifications
#define ELEVATOR_BASE_PORT 6000       // Base port for Elevators (each elevator: 6000 + id)
#define ELEVATOR_STATUS_PORT 50004    // Port for elevator status updates
#define STATUS_FAULT 3


std::map<int, ElevatorStatus> elevatorStatuses;

/**
 * @brief Constructor for the Scheduler.
 * Initializes the scheduler with the default floor request socket.
 */
Scheduler::Scheduler()
        : state(SchedulerState::IDLE), socket(FLOOR_REQUEST_PORT) {
    socketPtr = &socket;
}

/**
 * @brief Custom constructor for the Scheduler.
 * Allows for injecting a custom socket for UDP communication.
 * @param customSocket A pointer to a custom DatagramSocket.
 */
Scheduler::Scheduler(DatagramSocket* customSocket)
        : state(IDLE), socketPtr(customSocket) {}

/**
 * @brief Adds a new request to the scheduler's queue.
 * Places the request in either the "up" or "down" queue based on the requested direction.
 * Notifies the `processRequests` function immediately.
 * @param floor The floor where the request was made.
 * @param destinationFloor The destination floor for the request.
 * @param type Type of the request (floor or elevator).
 * @param source The source of the request (Floor or Elevator).
 * @param goingUp Whether the request is for moving up (true) or down (false).
 * @param elevatorId The ID of the elevator if the request comes from an elevator.
 * @param faultFlag The fault flag to determine if the elevator is faulty.
 */
void Scheduler::addRequest(int floor, int destinationFloor,
                           RequestType type, RequestSource source,
                           bool goingUp, int elevatorId,  int faultFlag) {
    std::unique_lock<std::mutex> lock(mtx);
    Request request(floor, destinationFloor, type, source, goingUp, elevatorId, 0, faultFlag);

    if (type == FLOOR_REQUEST) {
        if (goingUp) {
            upQueue.push(request);     // add to upQueue
        } else {
            downQueue.push(request);     // add to downQueue
        }
        cv.notify_one();  // notify processRequests() immediately
    }

    state = SchedulerState::PROCESSING;        // set state to PROCESSING
    std::cout << "[Scheduler] Added FLOOR REQUEST at Floor " << floor
              << " (Direction: " << (goingUp ? "Up" : "Down") << ") to queue"
              << std::endl;
}

/**
 * @brief Listens for incoming requests from the Floor system.
 * Receives UDP packets with floor requests and adds them to the scheduler queue.
 */
void Scheduler::listenForRequests() {
    while (true) {
        std::vector<uint8_t> in(sizeof(Request));
        DatagramPacket receivePacket(in, in.size());
        try {
            socket.receive(receivePacket); // Wait for floor request
        } catch (const std::runtime_error &e) {
            std::cerr << e.what() << std::endl;
            continue;
        }

        Request request;
        memcpy(&request, receivePacket.getData(), sizeof(Request));

        
        std::cout << "[Scheduler] Received request from Floor " << request.floor
                  << " (Direction: " << (request.goingUp ? "Up" : "Down") << ")"
                  << std::endl;
        // Add the request to the scheduler's queue
        addRequest(request.floor, request.destinationFloor, FLOOR_REQUEST, FROM_FLOOR, request.goingUp, -1, request.faultFlag);
    }
}

/**
 * @brief Receives periodic elevator status updates and detects faults.
 * Updates the scheduler with elevator location and status information.
 * @details Handles elevator heartbeat updates, completed trips, and fault reports.
 */
void Scheduler::receiveElevatorUpdates() {
    DatagramSocket socket(ELEVATOR_STATUS_PORT);

    while (true) {
        std::vector<uint8_t> data(sizeof(int) * 4); // Max needed (for STATUS_COMPLETE)
        DatagramPacket packet(data, data.size());

        try {
            socket.receive(packet); // Receive elevator update
        } catch (const std::runtime_error &e) {
            std::cerr << "[Scheduler] Error receiving elevator update: " << e.what() << std::endl;
            continue;
        }

        int elevatorId, value1, value2, statusType;
        memcpy(&elevatorId, data.data(), sizeof(int));
        memcpy(&value1, data.data() + sizeof(int), sizeof(int));
        memcpy(&value2, data.data() + 2 * sizeof(int), sizeof(int));
        memcpy(&statusType, data.data() + 3 * sizeof(int), sizeof(int));

        switch (static_cast<StatusType>(statusType)) {
            case STATUS_UPDATE: {
                int currentFloor = value1;
                elevatorFloors[elevatorId] = currentFloor;
                elevatorStatuses[elevatorId].state = ElevatorState::IDLE; // or MOVING if you add that info

                if (elevatorFaults[elevatorId]) {
                    elevatorFaults[elevatorId] = false;
                    std::cout << "[Scheduler] Elevator " << elevatorId << " is responsive again." << std::endl;
                }

                elevatorLastUpdate[elevatorId] = std::chrono::steady_clock::now();
                std::cout << "[Scheduler] Elevator " << elevatorId << " heartbeat: now at Floor " << currentFloor << std::endl;
                break;
            }

            case STATUS_COMPLETE: {
                int pickupFloor = value1;
                int destinationFloor = value2;
                std::cout << "[Scheduler] Elevator " << elevatorId << " completed trip: Pickup at Floor "
                          << pickupFloor << ", Destination Floor " << destinationFloor << std::endl;

                // Notify the floor subsystem about the completed trip
                notifyFloor(elevatorId, previousFloor[elevatorId], pickupFloor, destinationFloor);
                break;
            }
            case STATUS_FAULT: {
                std::cerr << "[Scheduler] ⚠️ FAULT reported by Elevator " << elevatorId << ". Marking as FAULTY." << std::endl;
                elevatorFaults[elevatorId] = true;
                break;
            }

            default:
                std::cerr << "[Scheduler] WARNING: Unknown statusType (" << statusType
                          << ") from Elevator " << elevatorId << std::endl;
                break;
        }
    }
}


/**
 * @brief Assigns the best available elevator to handle a floor request.
 * Skips faulty elevators and selects the one with the closest proximity and least load.
 * @param request The floor request that needs to be assigned to an elevator.
 */
void Scheduler::assignElevator(const Request& request) {
    int bestElevatorId = -1;
    int minDistance = INT_MAX;
    int minLoad = INT_MAX;

    for (const auto& entry : elevatorFloors) {
        int elevatorId = entry.first;
        int elevatorFloor = entry.second;

        // Skip faulty elevators
        if (elevatorFaults[elevatorId]  || elevatorStates[elevatorId] == ElevatorState::OUT_OF_SERVICE) {
            std::cout << "[Scheduler] Skipping elevator " << elevatorId << " due to FAULT or OUT_OF_SERVICE\n";
            continue;
        }

        int distance = abs(elevatorFloor - request.floor);
        int load = elevatorRequestQueue[elevatorId].size();
        bool movingInSameDirection = elevatorDirections.count(elevatorId) ?
                                     (request.goingUp == elevatorDirections[elevatorId]) : true;
        if (!movingInSameDirection) distance += 2; // Small penalty for opposite direction

        // Select the elevator with the least load and shortest distance
        if ((load < minLoad) || (load == minLoad && distance < minDistance)) {
            minLoad = load;
            minDistance = distance;
            bestElevatorId = elevatorId;
        }
    }

    if (bestElevatorId != -1) {
        previousFloor[bestElevatorId] = elevatorFloors[bestElevatorId];
        std::cout << "[Scheduler] Assigned Floor " << request.floor << " to Elevator " << bestElevatorId << std::endl;
        elevatorRequestQueue[bestElevatorId].push(request);
        elevatorDirections[bestElevatorId] = request.goingUp;

        pickupFloor[bestElevatorId] = request.floor;


        // Send request to the assigned elevator
        std::vector<uint8_t> data(sizeof(Request));
        memcpy(data.data(), &request, sizeof(Request));
        DatagramPacket sendPacket(data, data.size(),
                                  InetAddress::getLocalHost(), ELEVATOR_BASE_PORT + bestElevatorId);
        try {
            socket.send(sendPacket); // Send the request to the elevator
            std::cout << "[Scheduler] Sent assignment to Elevator " << bestElevatorId << std::endl;
        } catch (const std::runtime_error &e) {
            std::cerr << "[Scheduler] Error sending assignment: " << e.what() << std::endl;
        }
    } else {
        std::cerr << "[Scheduler] No available elevators for Floor " << request.floor << std::endl;
    }
}

/**
 * @brief Periodically checks for stuck elevators.
 * If no update is received in 15+ seconds, marks the elevator as faulty.
 */
void Scheduler::checkForStuckElevators() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        auto now = std::chrono::steady_clock::now();
        for (auto& entry : elevatorLastUpdate) {
            int elevatorId = entry.first;
            auto lastUpdateTime = entry.second;

            if (elevatorFaults[elevatorId]) continue;  // already marked faulty

            // NEW: Check if the elevator is idle — skip check if so
            if (elevatorStatuses[elevatorId].state == ElevatorState::IDLE) {
                // Optional: you can log that it's idle if needed
                continue;
            }

            // OLD: timeout-based hard fault detection
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdateTime).count() > 15) {
                std::cerr << "[Scheduler] WATCHDOG: Elevator " << elevatorId << " has not reported in 15s and is NOT idle! Marking as stuck." << std::endl;
                elevatorFaults[elevatorId] = true;

                // Optionally notify the GUI or print to terminal
            }
        }
    }
}


/**
 * @brief Processes incoming requests and assigns them to elevators.
 */
void Scheduler::processRequests() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        // Wait until there's a request or a shutdown signal
        cv.wait(lock, [this]() {
            return !upQueue.empty() || !downQueue.empty() || state == SchedulerState::SHUTTING_DOWN;
        });
        // If shutting down and no requests left, exit the loop
        if (state == SchedulerState::SHUTTING_DOWN && upQueue.empty() && downQueue.empty()) {
            std::cout << "[Scheduler] No more requests. Exiting..." << std::endl;
            return;
        }

        Request request;
        bool isFloorRequest = false;
        // Prioritize up-requests, then down-requests
        if (!upQueue.empty()) {
            request = upQueue.front();
            upQueue.pop();
            isFloorRequest = true;
        } else if (!downQueue.empty()) {
            request = downQueue.front();
            downQueue.pop();
            isFloorRequest = true;
        }
        lock.unlock();

        if (isFloorRequest) {


            std::cout << "[Scheduler] Processing FLOOR REQUEST at Floor " << request.floor
                      << " -> Destination " << request.destinationFloor
                      << " (" << (request.goingUp ? "Up" : "Down") << ")" << ", Fault Flag: " << request.faultFlag << std::endl;

            // Always assign elevator, even if fault flag == 1
            assignElevator(request);

        }
    }
}

/*
void Scheduler::handleFault(int elevatorId) {
    std::cout << "[Scheduler] Fault detected for Elevator " << elevatorId << std::endl;
    Elevator* elevator = elevators[elevatorId]; // Find the corresponding elevator
    if (elevator != nullptr) {
        elevator->setState(ElevatorState::FAULTY);
        // Additional logic to handle the fault (e.g., notify maintenance)
        std::cout << "[Scheduler] Elevator " << elevatorId << " marked as FAULTY." << std::endl;
    }
}
*/

/**
 * @brief Notifies the Floor subsystem when an elevator arrives.
 */
void Scheduler::notifyFloor(int elevatorId, int originFloor, int pickupFloor, int destinationFloor) {


    std::vector<uint8_t> data(sizeof(int) * 4);
    memcpy(data.data(), &elevatorId, sizeof(int));
    memcpy(data.data() + sizeof(int), &originFloor, sizeof(int));
    memcpy(data.data() + 2 * sizeof(int), &pickupFloor, sizeof(int));
    memcpy(data.data() + 3 * sizeof(int), &destinationFloor, sizeof(int));

    DatagramPacket packet(data, data.size(), InetAddress::getLocalHost(), FLOOR_NOTIFICATION_PORT);

    try {
        socketPtr->send(packet);
    } catch (const std::runtime_error &e) {
        std::cerr << "[Scheduler] Error sending floor notification: " << e.what() << std::endl;
    }
}





/**
 * @brief Stops the scheduler (shuts down processing).
 */
void Scheduler::stop() {
    std::unique_lock<std::mutex> lock(mtx);
    state = SchedulerState::SHUTTING_DOWN;
    cv.notify_all();
}



/**
 * @brief Main function to start the Scheduler system.
 */
#ifndef TEST_MODE
int main() {
    try {
        Scheduler scheduler;
        std::thread listenerThread(&Scheduler::listenForRequests, &scheduler);
        std::thread processingThread(&Scheduler::processRequests, &scheduler);
        std::thread updateThread(&Scheduler::receiveElevatorUpdates, &scheduler);
        std::thread faultCheckThread(&Scheduler::checkForStuckElevators, &scheduler);

        listenerThread.join();
        processingThread.join();
        updateThread.join();
        faultCheckThread.join();
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
    }
}
#endif
