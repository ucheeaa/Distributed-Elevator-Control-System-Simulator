/**
 * SYSC3303 - Project Iteration 3
 * Author: Nethul Bodiratne
 * Date: March 8th, 2025
 */

#include "Elevator.h"
#include <iostream>
#include <thread>
#include <unistd.h>
#include <chrono>
#include <cmath>

std::mutex logMutex;  // Global mutex for logging

#define ELEVATOR_STATUS_PORT 50004
#define SCHEDULER_PORT 50003
#define ELEVATOR_BASE_PORT 6000

/**
 * @class Elevator
 * @brief Constructor for the Elevator Class
 * @param unique elevator ID
 */
Elevator::Elevator(int elevatorId)
        : id(elevatorId), currentFloor(0), movingUp(true), isRunning(true), state(ElevatorState::IDLE), passengerCount(0),
          elevatorSocket(ELEVATOR_BASE_PORT + elevatorId) {
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

void Elevator::sendResponseToScheduler(int pickupFloor, int destinationFloor) {
    StatusType statusType = STATUS_COMPLETE;

    std::vector<uint8_t> data(sizeof(int) * 4);
    memcpy(data.data(), &id, sizeof(int));
    memcpy(data.data() + sizeof(int), &pickupFloor, sizeof(int));
    memcpy(data.data() + 2 * sizeof(int), &destinationFloor, sizeof(int));
    memcpy(data.data() + 3 * sizeof(int), &statusType, sizeof(int));

    DatagramPacket packet(data, data.size(), InetAddress::getLocalHost(), ELEVATOR_STATUS_PORT);
    try {
        elevatorSocket.send(packet);
        std::cout << "[Elevator " << id << "] Sent completion update to Scheduler." << std::endl;
    } catch (const std::runtime_error &e) {
        std::cerr << "[Elevator " << id << "] Error sending completion message: " << e.what() << std::endl;
    }
}




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

int Elevator::getCurrentFloor() const {
    return currentFloor;
}

bool Elevator::isIdle() const {
    return requestQueue.empty() && state == ElevatorState::IDLE;
}

bool Elevator::isMovingUp() const {
    return state == ElevatorState::MOVING_UP;
}

bool Elevator::isMovingDown() const {
    return state == ElevatorState::MOVING_DOWN;
}
/**
void Elevator::simulateMoving(int destinationFloor) {
    double a = 0.0;
    double maxV = 0.0;
    const double floorHeight = 4.0;

    if (std::abs(currentFloor - destinationFloor) > 1) {
        a = 0.0557;
        maxV = 0.4720;
    } else {
        a = 0.0493;
        maxV = 0.6281;
    }

    double position = currentFloor * floorHeight;
    double targetPosition = destinationFloor * floorHeight;
    double speed = 0.0;
    int direction = (isMovingUp()) ? 1 : -1;

    while ((isMovingUp() && position < targetPosition) || (isMovingDown() && position > targetPosition)) {
        double timeStep = 0.01;

        if (std::abs(position - targetPosition) > maxV * maxV / a) {
            speed = std::min(speed + a * timeStep, maxV);
        } else {
            speed = std::max(speed - a * timeStep, 0.0);
        }

        position += direction * speed * timeStep;

        std::cout << "At height: " << position << " meters (~Floor "
                  << round(position / floorHeight) << ") - Speed: "
                  << speed << " m/s\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
*/



void Elevator::simulateMoving(int destinationFloor) {
    const double floorHeight = 4.0;
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

    // Simulate passing each floor
    while (currentFloor != destinationFloor) {
        currentFloor += direction;

        // Simulate time delay per floor
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((travelTime / floorsToMove) * 1000)));

        std::cout << "[Elevator " << id << "] Passing Floor " << currentFloor << std::endl;

        // 🔔 Send a live status update to Scheduler every time floor is passed!
        sendElevatorStatus();
    }

    std::cout << "[Elevator " << id << "] Arrived at Destination Floor " << currentFloor << "\n";
}




void Elevator::run() {
    while (isRunning) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !requestQueue.empty(); });


        if (!isRunning) break;

        if (requestQueue.empty()) {
            std::cout << "[Elevator " << id << "] WARNING: Queue is empty, waiting for requests..." << std::endl;
            continue;
        }

        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        std::cout << "[Elevator " << id << "] PROCESSING REQUEST: Floor " << req.floor
                  << " -> Destination " << req.destinationFloor << std::endl;

        // Start movement in separate thread
        bool arrivedOnTime = false;
        std::thread movementThread([&]() {
            simulateMoving(req.destinationFloor);
            arrivedOnTime = true;
        });

        // Calculate estimated travel time
        double estimatedTravelTime = 0.0;
        int floorsToMove = std::abs(currentFloor - req.destinationFloor);
        if (floorsToMove == 1) {
            estimatedTravelTime = 8.475;
        } else if (floorsToMove >= 2 && floorsToMove <= 3) {
            estimatedTravelTime = 12.737;
        } else {
            estimatedTravelTime = 12.737 + ((floorsToMove - 2) * 4.0);
        }

        // Sleep for expected time + buffer
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((estimatedTravelTime + 3.0) * 1000)));

        movementThread.join();

        // Detect time-based hard fault
        if (!arrivedOnTime) {
            std::cerr << "[Elevator " << id << "] HARD FAULT: Floor timer fault detected! Shutting down elevator." << std::endl;
            state = ElevatorState::OUT_OF_SERVICE;
            sendFaultToScheduler();  // Notify Scheduler
            stop(); // Stop elevator thread loop
            return;
        }

        // Handle door stuck fault (transient)
        if (req.faultFlag == 1) {
            std::cerr << "[Elevator " << id << "] TRANSIENT FAULT: DOOR STUCK detected at Floor " << currentFloor << std::endl;
            int retries = 3;
            while (retries--) {
                std::cout << "[Elevator " << id << "] Retrying door operation (" << (3 - retries) << "/3)...\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::cout << "[Elevator " << id << "] Door fault cleared after retries.\n";
        }

        // Handle manually injected hard fault
        if (req.faultFlag == 2) {
            std::cerr << "[Elevator " << id << "] HARD FAULT (Manual Inject): Floor timer fault detected! Shutting down elevator." << std::endl;
            state = ElevatorState::OUT_OF_SERVICE;
            sendFaultToScheduler();  // Notify Scheduler
            stop(); // Stop elevator thread loop
            return;
        }

        // Simulate normal door operation
        state = ElevatorState::DOOR_OPEN;
        std::cout << "[Elevator " << id << "] Doors opening..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "[Elevator " << id << "] Doors closing..." << std::endl;
        state = ElevatorState::IDLE;

        sendResponseToScheduler(req.floor, req.destinationFloor);
        std::cout << "[Elevator " << id << "] Trip completed. Now idle at Floor " << currentFloor << std::endl;
    }
}



void Elevator::sendFaultToScheduler() {
    StatusType statusType = STATUS_FAULT;

    std::vector<uint8_t> data(sizeof(int) * 4);
    memcpy(data.data(), &id, sizeof(int));
    memcpy(data.data() + sizeof(int), &currentFloor, sizeof(int));
    int dummy = 0;
    memcpy(data.data() + 2 * sizeof(int), &dummy, sizeof(int)); // Not needed, filler
    memcpy(data.data() + 3 * sizeof(int), &statusType, sizeof(int));

    DatagramPacket packet(data, data.size(), InetAddress::getLocalHost(), ELEVATOR_STATUS_PORT);
    try {
        elevatorSocket.send(packet);
        std::cout << "[Elevator " << id << "] Sent FAULT status to Scheduler." << std::endl;
    } catch (const std::runtime_error &e) {
        std::cerr << "[Elevator " << id << "] Error sending FAULT status: " << e.what() << std::endl;
    }
}



void Elevator::simulateDoorOperation(int faultCode) {
    std::cout << "[Elevator " << id << "] Doors opening..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (faultCode == 1) { // Simulate door stuck fault
        std::cerr << "[Elevator " << id << "] ⚠️ WARNING: Door stuck open! Retrying...\n";
        // Retry logic
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << "[Elevator " << id << "] Doors closing (retry)..." << std::endl;
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "[Elevator " << id << "] Doors closing..." << std::endl;
    }
}




void Elevator::sendElevatorStatus() {
    {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "[Elevator " << id << "] Preparing to send floor update: " << currentFloor << std::endl;
    }

    StatusType statusType = STATUS_UPDATE;

    int dummy = 0;

    std::vector<uint8_t> data(sizeof(int) * 4);
    memcpy(data.data(), &id, sizeof(int));
    memcpy(data.data() + sizeof(int), &currentFloor, sizeof(int));
    memcpy(data.data() + 2 * sizeof(int), &dummy, sizeof(int));  // placeholder
    memcpy(data.data() + 3 * sizeof(int), &statusType, sizeof(int));

    {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "[Elevator " << id << "]  Sending Status: ID=" << id
                  << ", Floor=" << currentFloor << " | Raw Data: ";
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
 * @brief Periodically sends elevator status (heartbeat) to Scheduler.
 */
void Elevator::statusUpdateLoop() {
    while (isRunning) {
        sendElevatorStatus();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

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
