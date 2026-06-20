/**
 * SYSC3303 - Project Iteration 5
 * Group: B2-G5
 * Date: April 7th, 2025
 */

#include "Elevator.h"
#include <iostream>
#include <thread>
#include <unistd.h>
#include <chrono>
#include <cmath>
#include <algorithm>

std::mutex logMutex;  // Global mutex for logging

#define ELEVATOR_STATUS_PORT 50004
#define SCHEDULER_PORT 50003
#define ELEVATOR_BASE_PORT 6100
#define ELEVATOR_RETRY_PORT 50006
const int MAX_CAPACITY = 4;

/**
 * @class Elevator
 * @brief Constructor for the Elevator Class
 * @param unique elevator ID
 */
Elevator::Elevator(int elevatorId)
        : id(elevatorId), currentFloor(0), movingUp(true), isRunning(true), state(ElevatorState::IDLE), passengerCount(0),
          elevatorSocket(ELEVATOR_BASE_PORT + elevatorId),totalMovements(0), startTime(std::chrono::steady_clock::now()) {
    std::cout << "[Elevator " << id << "] Ready on port " << (ELEVATOR_BASE_PORT + id) << "\n";
}

/**
 * @brief Adds request to elevator queue.
 * @param The request that needs to be added to the queue.
 */
void Elevator::addRequest(const Request& request) {
    std::unique_lock<std::mutex> lock(mtx);
    requestQueue.push(request);
    cv.notify_one();
}

/**
 * @brief Sends STATUS_COMPLETE to Scheduler.
 * @param The floor the Elevator started at.
 * @param The floor that sent the elevator request.
 * @param The destination floor of the request.
 */
void Elevator::sendResponseToScheduler(int origin, int pickupFloor, int destinationFloor) {
    StatusType statusType = STATUS_COMPLETE;

    std::vector<uint8_t> data(sizeof(int) * 5);
    memcpy(data.data(), &id, sizeof(int));
    memcpy(data.data() + sizeof(int), &origin, sizeof(int));
    memcpy(data.data() + 2 * sizeof(int), &pickupFloor, sizeof(int));
    memcpy(data.data() + 3 * sizeof(int), &destinationFloor, sizeof(int));
    memcpy(data.data() + 4 * sizeof(int), &statusType, sizeof(int));

    DatagramPacket packet(data, data.size(), InetAddress::getLocalHost(), ELEVATOR_STATUS_PORT);
    try {
        elevatorSocket.send(packet);
        std::cout << "[Elevator " << id << "] Sent STATUS_COMPLETE to Scheduler." << std::endl;
    } catch (...) {
        std::cerr << "[Elevator " << id << "] Failed to send STATUS_COMPLETE.\n";
    }

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();
    std::cout << "[Elevator " << id << "] Total Movements: " << totalMovements << std::endl;
    std::cout << "[Elevator " << id << "] Total Time Taken: " << duration << "s" << std::endl;
}

/**
 * @brief Listens to the scheduler for new requests and adds to the queue.
 */
void Elevator::listenForRequests() {
    std::vector<uint8_t> buffer(sizeof(Request));
    DatagramPacket receivePacket(buffer, buffer.size());

    while (isRunning) {
        try {
            elevatorSocket.receive(receivePacket);
            if (receivePacket.getLength() == sizeof(Request)) {
                Request request;
                memcpy(&request, receivePacket.getData(), sizeof(Request));

                std::cout << "[Elevator " << id << "] Parsed request: Floor "
                          << request.floor << " -> Destination " << request.destinationFloor
                          << " (" << (request.goingUp ? "Up" : "Down") << ")" << std::endl;

                addRequest(request);
            } else {
                std::cerr << "[Elevator " << id << "] ERROR: Received data size mismatch." << std::endl;
            }
        } catch (const std::runtime_error &e) {
            std::cerr << "[Elevator " << id << "] Error receiving request: " << e.what() << std::endl;
        }
    }
}

/**
 * @brief Gets the current floor the elevator is on.
 * @return Current floor int.
 */
int Elevator::getCurrentFloor() const {
    return currentFloor;
}

/**
 * @brief Checks if the elevator is idle.
 * @return True if idle, false otherwise.
 */
bool Elevator::isIdle() const {
    return requestQueue.empty() && state == ElevatorState::IDLE;
}

/**
 * @brief Checks if elevator is moving up.
 * @return True if moving up, false otherwise.
 */
bool Elevator::isMovingUp() const {
    return state == ElevatorState::MOVING_UP;
}

/**
 * @brief Checks if elevator is moving down.
 * @return True if moving down, false otherwise.
 */
bool Elevator::isMovingDown() const {
    return state == ElevatorState::MOVING_DOWN;
}


/**
 * @brief Simulates the movement of the elevator between floors.
 * @param The destination floor int.
 */
void Elevator::simulateMoving(int destinationFloor) {
    if (destinationFloor == currentFloor) {
        std::cout << "[Elevator " << id << "] Already at destination floor " << destinationFloor << ". No movement needed.\n";
        state = ElevatorState::IDLE;
        return;
    }

    int floorsToMove = std::abs(destinationFloor - currentFloor);
    double travelTime = 0.0;

    if (floorsToMove == 1) {
        travelTime = 8.475;
    } else if (floorsToMove >= 2 && floorsToMove <= 3) {
        travelTime = 12.737;
    } else {
        travelTime = 12.737 + ((floorsToMove - 2) * 4.0);
    }

    std::cout << "[Elevator " << id << "] Starting trip from Floor " << currentFloor
              << " to Floor " << destinationFloor << " (Approx " << travelTime << "s)\n";

    int direction = (destinationFloor > currentFloor) ? 1 : -1;

    if (direction == 1) {
        state = ElevatorState::MOVING_UP;
    } else {
        state = ElevatorState::MOVING_DOWN;
    }

    for (int i = 0; i < floorsToMove; ++i) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            currentFloor += direction;
        }
        totalMovements++;

        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((travelTime / floorsToMove) * 1000)));
        std::cout << "[Elevator " << id << "] Passing Floor " << currentFloor << std::endl;

        sendElevatorStatus();
    }

    std::cout << "[Elevator " << id << "] Arrived at Destination Floor " << destinationFloor << "\n";
}


/**
 * @brief Resends button press to Scheduler after full capacity.
 * @param Original request.
 */
void Elevator::resendRequestToScheduler(const Request& originalRequest) {
    std::cout << "[Elevator " << id << "] Button pressed again at Floor "
              << originalRequest.floor << " (Direction: "
              << (originalRequest.goingUp ? "Up" : "Down") << ")" << std::endl;

    // Create a copy of the request and set the correct metadata
    Request retryRequest = originalRequest;
    retryRequest.type = ELEVATOR_REQUEST;
    retryRequest.source = FROM_ELEVATOR;
    retryRequest.elevatorId = id;

    DatagramSocket socket;
    std::vector<uint8_t> data(sizeof(Request));
    memcpy(data.data(), &retryRequest, sizeof(Request));

    DatagramPacket packet(data, data.size(), InetAddress::getLocalHost(), ELEVATOR_RETRY_PORT);

    try {

        Request debugRequest;
        memcpy(&debugRequest, data.data(), sizeof(Request));
        socket.send(packet);
        std::cout << "[Elevator " << id << "] Re-sent request to Scheduler for Floor "
                  << retryRequest.floor << " -> " << retryRequest.destinationFloor << std::endl;
    } catch (const std::runtime_error &e) {
        std::cerr << "[Elevator " << id << "] Failed to resend request: " << e.what() << std::endl;
    }
}


/**
 * @brief Main loop for elevator operation.
 */
void Elevator::run() {
    std::cout << "[Elevator " << id << "] Starting elevator run loop..." << std::endl;

    while (isRunning) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !requestQueue.empty(); });

        if (!isRunning) break;
        if (requestQueue.empty()) continue;

        Request req = requestQueue.front();
        requestQueue.pop();

        if (passengerCount >= MAX_CAPACITY) {
            std::cout << "[Elevator " << id << "] Capacity full! Re-sending request from Floor "
                      << req.floor << " to Scheduler.\n";
            resendRequestToScheduler(req);
            continue;
        }

        int frozenOriginFloor = currentFloor;
        lock.unlock();

        std::cout << "[Elevator " << id << "] PROCESSING REQUEST: Floor " << req.floor
                  << " -> Destination " << req.destinationFloor << std::endl;

        // 🕒 Estimate travel time (pickup + destination)
        int totalFloors = std::abs(currentFloor - req.floor) + std::abs(req.floor - req.destinationFloor);
        double estimatedTravelTime;
        if (totalFloors == 1) estimatedTravelTime = 8.475;
        else if (totalFloors <= 3) estimatedTravelTime = 12.737;
        else estimatedTravelTime = 12.737 + ((totalFloors - 2) * 4.0);
        double bufferTime = 4.0;

        // Inject hard fault via delay (simulate floor timeout)
        if (req.faultFlag == 2) {
            std::cerr << "[Elevator " << id << "] Injected HARD FAULT: Simulating timeout delay...\n";
            std::this_thread::sleep_for(std::chrono::seconds(static_cast<int>(estimatedTravelTime + bufferTime + 2)));

            resendRequestToScheduler(req);

            std::cerr << "[Elevator " << id << "]  HARD FAULT: Injected fault triggered shutdown.\n";
            state = ElevatorState::OUT_OF_SERVICE;
            sendFaultToScheduler(STATUS_HARD_FAULT);

            for (const Request& r : activePassengers) {
                resendRequestToScheduler(r);
            }
            while (!requestQueue.empty()) {
                resendRequestToScheduler(requestQueue.front());
                requestQueue.pop();
            }

            stop();
            return;
        }

        // NORMAL execution if no hard fault
        std::thread movementThread([&]() {
            simulateMoving(req.floor);

            // Gather requests at pickup floor
            requestsAtThisFloor.clear();
            requestsAtThisFloor.push_back(req);
            std::queue<Request> tempQueue;

            while (!requestQueue.empty()) {
                Request next = requestQueue.front();
                requestQueue.pop();

                if (next.floor == req.floor) {
                    requestsAtThisFloor.push_back(next);
                } else {
                    tempQueue.push(next);
                }
            }
            std::swap(requestQueue, tempQueue);

            // Pickup passengers
            state = ElevatorState::DOOR_OPEN;
            std::cout << "[Elevator " << id << "] Doors opening at Pickup Floor " << req.floor << "...\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            handlePickupFloor(req.floor);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "[Elevator " << id << "] Doors closing...\n";
            state = ElevatorState::IDLE;

            // Drop off passengers
            for (const Request& r : activePassengers) {
                simulateMoving(r.destinationFloor);
                state = ElevatorState::DOOR_OPEN;
                std::cout << "[Elevator " << id << "] Doors opening at Floor " << r.destinationFloor << " for drop-off.\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));

                std::cout << "[Elevator " << id << "] Passenger dropped off at Floor " << r.destinationFloor << "\n";
                passengerCount--;
                sendResponseToScheduler(frozenOriginFloor, r.floor, r.destinationFloor);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "[Elevator " << id << "] Doors closing...\n";
                state = ElevatorState::IDLE;

                // Handle transient fault
                if (r.faultFlag == 1) {
                    std::cerr << "[Elevator " << id << "] TRANSIENT FAULT: DOOR STUCK at Floor " << r.destinationFloor << "\n";
                    for (int i = 1; i <= 3; ++i) {
                        std::cout << "[Elevator " << id << "] Retrying door operation (" << i << "/3)...\n";
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                    std::cout << "[Elevator " << id << "] Door fault cleared after retries.\n";
                    sendFaultToScheduler(STATUS_TRANSIENT_FAULT);
                }
            }

            activePassengers.clear();
        });

        // Wait for normal movement thread to finish
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((estimatedTravelTime + bufferTime) * 1000)));
        movementThread.join();

        // Wrap up trip
        state = ElevatorState::DOOR_OPEN;
        std::cout << "[Elevator " << id << "] Doors opening...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "[Elevator " << id << "] Doors closing...\n";
        state = ElevatorState::IDLE;

        std::cout << "[Elevator " << id << "] Trip completed. Now idle at Floor " << currentFloor << "\n";
    }
}


/**
 * @brief Sends STATUS_FAULT to scheduler.
 */
void Elevator::sendFaultToScheduler(StatusType faultType) {
    int origin = currentFloor;
    int pickup = -1;
    int dest = -1;

    std::vector<uint8_t> data(sizeof(int) * 5);
    memcpy(data.data(), &id, sizeof(int));
    memcpy(data.data() + sizeof(int), &origin, sizeof(int));
    memcpy(data.data() + 2 * sizeof(int), &pickup, sizeof(int));
    memcpy(data.data() + 3 * sizeof(int), &dest, sizeof(int));
    memcpy(data.data() + 4 * sizeof(int), &faultType, sizeof(int));

    DatagramPacket packet(data, data.size(), InetAddress::getLocalHost(), ELEVATOR_STATUS_PORT);
    try {
        elevatorSocket.send(packet);
        std::string msg = (faultType == STATUS_HARD_FAULT)
                          ? "STATUS_HARD_FAULT"
                          : (faultType == STATUS_TRANSIENT_FAULT ? "STATUS_TRANSIENT_FAULT" : "UNKNOWN_FAULT");
        std::cout << "[Elevator " << id << "] Sent " << msg << " to Scheduler." << std::endl;
    } catch (...) {
        std::cerr << "[Elevator " << id << "] Failed to send fault status.\n";
    }
}

/**
 * @return Current number of passengers onboard.
 */
int Elevator::getPassengerCount() const {
    return passengerCount;
}

/**
 * @brief Simulates door opening.
 * @param Fault code, 0 for no fault, 1 for door stuck.
 */
void Elevator::simulateDoorOperation(int faultCode) {
    std::cout << "[Elevator " << id << "] Doors opening..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (faultCode == 1) { // Simulate door stuck fault
        std::cerr << "[Elevator " << id << "] WARNING: Door stuck open! Retrying...\n";
        // Retry logic
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << "[Elevator " << id << "] Doors closing (retry)..." << std::endl;
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "[Elevator " << id << "] Doors closing..." << std::endl;
    }
}

/**
 * @brief Handles request pick-up at a given floor considering capacity limits.
 * @param currentPickupFloor The floor where pick-up is occurring.
 */
 void Elevator::handlePickupFloor(int currentPickupFloor) {
    std::vector<Request> remainingRequests;
    bool pickedUp = false;

    for (auto& req : requestsAtThisFloor) {
        if (req.floor == currentPickupFloor) {
            if (passengerCount < MAX_CAPACITY) {
                activePassengers.push_back(req);
                passengerCount++;
                pickedUp = true;

                std::cout << "[Elevator " << id << "] Picked up request: Floor " << req.floor
                          << " -> Dest " << req.destinationFloor
                          << " (" << (req.goingUp ? "Up" : "Down") << ")\n";
            } else {
                resendRequestToScheduler(req);
            }
        } else {
            remainingRequests.push_back(req); // leave it in the queue
        }
    }

    requestsAtThisFloor = remainingRequests;

    if (!pickedUp) {
        std::cout << "[Elevator " << id << "]  No one boarded at Floor " << currentPickupFloor << "\n";
    }

    std::cout << "[Elevator " << id << "] Current load: " << passengerCount << "/" << MAX_CAPACITY << "\n";
}

/**
 * @brief Sends a periodic STATUS_UPDATE to the Scheduler.
 */
void Elevator::sendElevatorStatus() {
    StatusType statusType = STATUS_UPDATE;
    int origin = currentFloor;
    int pickup = -1;
    int dest = -1;

    std::vector<uint8_t> data(sizeof(int) * 5);
    memcpy(data.data(), &id, sizeof(int));
    memcpy(data.data() + sizeof(int), &origin, sizeof(int));
    memcpy(data.data() + 2 * sizeof(int), &pickup, sizeof(int));
    memcpy(data.data() + 3 * sizeof(int), &dest, sizeof(int));
    memcpy(data.data() + 4 * sizeof(int), &statusType, sizeof(int));

    DatagramPacket packet(data, data.size(), InetAddress::getLocalHost(), ELEVATOR_STATUS_PORT);
    try {
        elevatorSocket.send(packet);
        {
            std::lock_guard<std::mutex> logLock(logMutex);
            std::cout << "[Elevator " << id << "] Sent STATUS_UPDATE: Floor = " << currentFloor << std::endl;
        }

    } catch (...) {
        std::cerr << "[Elevator " << id << "] Failed to send STATUS_UPDATE.\n";
    }
}

/**
 * @brief Periodically sends elevator status (heartbeat) to Scheduler.
 */
void Elevator::statusUpdateLoop() {
    while (isRunning) {
        sendElevatorStatus();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}


/**
 * @brief Sets the current floor of the elevator.
 * @param floor The floor to set as the current floor of the elevator.
 */
void Elevator::setCurrentFloor(int floor) {
    currentFloor = floor;
}

/**
 * @brief Gets the unique ID of the elevator.
 * @return The elevator's unique ID.
 */
int Elevator::getId() const {
    return id;
}

/**
 * @brief Gets the current state of the elevator.
 * @return The current state of the elevator, which is one of the values from the ElevatorState enum.
 */
ElevatorState Elevator::getState() const {
    return state;  // Return the ElevatorState enum directly
}


/**
 * @brief Stops elevator operation.
 */
void Elevator::stop() {
    std::unique_lock<std::mutex> lock(mtx);
    isRunning = false;
    cv.notify_all();
}

#ifndef TEST_MODE
int main() {
    Elevator elevator1(1);
    Elevator elevator2(2);
    Elevator elevator3(3);
    Elevator elevator4(4);

    std::thread listenerThread1(&Elevator::listenForRequests, &elevator1);
    std::thread listenerThread2(&Elevator::listenForRequests, &elevator2);
    std::thread listenerThread3(&Elevator::listenForRequests, &elevator3);
    std::thread listenerThread4(&Elevator::listenForRequests, &elevator4);

    std::thread elevatorThread1(&Elevator::run, &elevator1);
    std::thread elevatorThread2(&Elevator::run, &elevator2);
   std::thread elevatorThread3(&Elevator::run, &elevator3);
   std::thread elevatorThread4(&Elevator::run, &elevator4);

    std::thread statusThread1(&Elevator::statusUpdateLoop, &elevator1);
    std::thread statusThread2(&Elevator::statusUpdateLoop, &elevator2);
    std::thread statusThread3(&Elevator::statusUpdateLoop, &elevator3);
    std::thread statusThread4(&Elevator::statusUpdateLoop, &elevator4);

    listenerThread1.join();
    listenerThread2.join();
    listenerThread3.join();
    listenerThread4.join();

    elevatorThread1.join();
   elevatorThread2.join();
    elevatorThread3.join();
    elevatorThread4.join();

    statusThread1.join();
    statusThread2.join();
    statusThread3.join();
    statusThread4.join();

    return 0;
}
#endif
