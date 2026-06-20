/**
 * SYSC3303 - Project Iteration 3
 * Author: Nethul Bodiratne
 * Date: March 8th, 2025
 */

#include "Elevator.h"
#include <iostream>
#include <thread>
#include <unistd.h>


std::mutex logMutex;  // Global mutex for logging


// Define the IP and Port for communication with the Scheduler
#define ELEVATOR_STATUS_PORT 50004  // New port for elevator status updates
#define SCHEDULER_PORT 50003        // Keep this for elevator completion updates
#define ELEVATOR_BASE_PORT 6000   // Base port for Elevators (Each elevator gets a unique port: 6000 + id)

/**
 * @brief Constructor for the Elevator class.
 * @param elevatorId Unique identifier for the elevator.
 */
Elevator::Elevator(int elevatorId)
        : id(elevatorId), currentFloor(0), movingUp(true), isRunning(true), state(ElevatorState::IDLE), passengerCount(0),
          elevatorSocket(ELEVATOR_BASE_PORT + elevatorId) { // Initialize the DatagramSocket with the unique port
    std::cout << "[Elevator " << id << "] Ready on port " << (ELEVATOR_BASE_PORT + id) << "\n";
}

/**
 * @brief Adds a request to the elevator's queue.
 * @param request The request to be processed.
 */
void Elevator::addRequest(const Request& request) {
    std::unique_lock<std::mutex> lock(mtx); // Ensure thread safety
    requestQueue.push(request);
    cv.notify_one(); // Notify the elevator thread that a new request is available
}

/**
 * @brief Sends a UDP message to the Scheduler when a request is completed.
 * @param floor The floor the elevator reached.
 * @param destinationFloor The destination floor the passenger requested.
 */
void Elevator::sendResponseToScheduler(int floor, int destinationFloor) {
    Request request;
    request.floor = floor;
    request.destinationFloor = destinationFloor;
    request.type = FLOOR_REQUEST; // Set the appropriate type
    request.source = FROM_ELEVATOR; // Set the appropriate source
    request.goingUp = (destinationFloor > floor); // Determine direction
    request.elevatorId = id;

    std::vector<uint8_t> data(sizeof(Request));
    memcpy(data.data(), &request, sizeof(Request)); // Use memcpy to copy the entire struct

    DatagramPacket packet(data, data.size(), InetAddress::getLocalHost(), SCHEDULER_PORT);
    try {
        elevatorSocket.send(packet);
        std::cout << "[Elevator " << id << "] Sent completion update to Scheduler." << std::endl;
    } catch (const std::runtime_error &e) {
        std::cerr << "[Elevator " << id << "] Error sending message: " << e.what() << std::endl;
    }
}

/**
 * @brief Listens for requests from the Scheduler over UDP.
 *
 * This function runs in a separate thread and waits for elevator assignments.
 * It parses incoming requests and adds them to the elevator's queue.
 */
void Elevator::listenForRequests() {
    std::vector<uint8_t> buffer(sizeof(Request));  // Create a buffer to store received messages
    DatagramPacket receivePacket(buffer, buffer.size());

    while (isRunning) {
        try {
            elevatorSocket.receive(receivePacket);
            if (receivePacket.getLength() == sizeof(Request)) {
                Request request;
                memcpy(&request, receivePacket.getData(), sizeof(Request)); // Use memcpy to copy the struct

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
 * @brief Retrieves the current floor of the elevator.
 * @return The current floor number.
 */
int Elevator::getCurrentFloor() const {
    return currentFloor;
}

/**
 * @brief Checks if the elevator is idle.
 * @return True if the request queue is empty, false otherwise.
 */
bool Elevator::isIdle() const {
    return requestQueue.empty() && state == Elevator::ElevatorState::IDLE;
}

/**
 * @brief Checks if the elevator is moving up.
 * @return True if the elevator is moving up, false otherwise.
 */
bool Elevator::isMovingUp() const {
    return state == Elevator::ElevatorState::MOVING_UP;
}

/**
 * @brief Checks if the elevator is moving down.
 * @return True if the elevator is moving down, false otherwise.
 */
bool Elevator::isMovingDown() const {
    return state == Elevator::ElevatorState::MOVING_DOWN;
}

/**
 * @brief Main loop for elevator operation.
 */
void Elevator::run() {
    while (isRunning) {
        sendElevatorStatus();

        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !requestQueue.empty() || !isRunning; });

        if (!isRunning) {
            std::cout << "[Elevator " << id << "] Stopping and exiting thread.\n";
            return;
        }

        if (requestQueue.empty()) {
            std::cout << "[Elevator " << id << "] WARNING: Queue is empty, waiting for requests..." << std::endl;
            continue;
        }

        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        std::cout << "[Elevator " << id << "] PROCESSING REQUEST: Floor " << req.floor
                  << " -> Destination " << req.destinationFloor << std::endl;

        if (req.goingUp) {
            state = Elevator::ElevatorState::MOVING_UP;
            std::cout << "[Elevator " << id << "] Moving UP to floor " << req.destinationFloor << std::endl;
        } else {
            state = Elevator::ElevatorState::MOVING_DOWN;
            std::cout << "[Elevator " << id << "] Moving DOWN to floor " << req.destinationFloor << std::endl;
        }

        currentFloor = req.destinationFloor;
        std::cout << "[Elevator " << id << "] Reached floor " << currentFloor << std::endl;

        state = Elevator::ElevatorState::DOOR_OPEN;
        std::cout << "[Elevator " << id << "] Doors opening..." << std::endl;
        sleep(1);

        std::cout << "[Elevator " << id << "] Doors closing..." << std::endl;
        state = Elevator::ElevatorState::IDLE;

        sendResponseToScheduler(currentFloor, req.destinationFloor);
    }
}
void Elevator::sendElevatorStatus() {
    {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "[Elevator " << id << "] Preparing to send floor update: " << currentFloor << std::endl;
    }

    std::vector<uint8_t> data(sizeof(int) * 2);
    memcpy(data.data(), &id, sizeof(int));
    memcpy(data.data() + sizeof(int), &currentFloor, sizeof(int));

    {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "[Elevator " << id << "]  Sending Status: ID=" << id
                  << ", Floor=" << currentFloor << " | Raw Data: ";
    }

    {
        std::lock_guard<std::mutex> lock(logMutex);
        for (size_t i = 0; i < data.size(); ++i) {
            std::cout << std::hex << (int)data[i] << " ";
        }
        std::cout << std::endl;
    }

    DatagramPacket packet(data, data.size(), InetAddress::getLocalHost(), ELEVATOR_STATUS_PORT);
    try {
        elevatorSocket.send(packet);
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "[Elevator " << id << "] Sent floor update: Current Floor = " << currentFloor << std::endl;
    } catch (const std::runtime_error &e) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cerr << "[Elevator " << id << "] Error sending floor update: " << e.what() << std::endl;
    }
}



/**
 * @brief Stops the elevator operation and closes UDP socket.
 *
 * This function sets the `isRunning` flag to false and
 * notifies the condition variable to ensure the thread exits.
 */
void Elevator::stop() {
    std::unique_lock<std::mutex> lock(mtx);
    isRunning = false;
    cv.notify_all();  // Notify any waiting threads to exit
}

#ifndef TEST_MODE
int main() {
    // Initialize four elevators (IDs 1 to 4)
    Elevator elevator1(1);
    Elevator elevator2(2);
    Elevator elevator3(3);
    Elevator elevator4(4);

    // Start listening for requests in separate threads
    std::thread listenerThread1(&Elevator::listenForRequests, &elevator1);
    std::thread listenerThread2(&Elevator::listenForRequests, &elevator2);
    std::thread listenerThread3(&Elevator::listenForRequests, &elevator3);
    std::thread listenerThread4(&Elevator::listenForRequests, &elevator4);

    // Start the elevator movement loops in separate threads
    std::thread elevatorThread1(&Elevator::run, &elevator1);
    std::thread elevatorThread2(&Elevator::run, &elevator2);
    std::thread elevatorThread3(&Elevator::run, &elevator3);
    std::thread elevatorThread4(&Elevator::run, &elevator4);

    // Ensure all threads continue running in parallel
    listenerThread1.join();
    listenerThread2.join();
    listenerThread3.join();
    listenerThread4.join();

    elevatorThread1.join();
    elevatorThread2.join();
    elevatorThread3.join();
    elevatorThread4.join();

    return 0;
}
#endif
