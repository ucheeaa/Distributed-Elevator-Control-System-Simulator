/**
 * SYSC3303 - Project Iteration 1 
 * Author: Uchenna Obikwelu
 * Date: February 1st, 2025
 */

#include <iostream>
#include <thread>
#include "Scheduler.h"
#include <climits>
#include "Elevator.h"
#include "Floor.h"
#include <optional>
#include <mutex>

/**
 * @brief Constructor for the Scheduler.
 * @param elevators A vector of pointers to Elevator objects.
 * @param floorSubsystem A pointer to the Floor subsystem.
 */
Scheduler::Scheduler(std::vector<Elevator*>& elevators, Floor* floorSubsystem)
        : elevators(elevators), floorSubsystem(floorSubsystem) {}

/**
 * @brief Adds a new request to the scheduler's queue.
 * @param floor The target floor for the request.
 * @param type The type of request (Floor/Elevator).
 * @param source The source of the request (From Floor/Elevator).
 * @param goingUp True for "Up" requests, False for "Down".
 * @param elevatorId The ID of the elevator (relevant for elevator requests).
 */
void Scheduler::addRequest(int floor, RequestType type, RequestSource source, bool goingUp, int elevatorId) {
    std::unique_lock<std::mutex> lock(mtx);

    Request request(floor, type, source, goingUp, elevatorId);

    if (type == FLOOR_REQUEST) {
        if (goingUp) {
            upQueue.push(request);
        } else {
            downQueue.push(request);
        }
        cv.notify_one(); // Notify processRequests to handle the request.
    } else if (type == ELEVATOR_REQUEST) {
        elevatorQueue.push(request);
        cv.notify_one(); // Notify processRequests to handle the request.
    }
}

/**
 * @brief Assigns the best elevator to handle the given request.
 * @param request The request to be assigned.
 */
void Scheduler::assignElevator(const Request& request) {
    Elevator* bestElevator = nullptr;
    int minDistance = INT_MAX;

    for (Elevator* elevator : elevators) {
        int distance = abs(elevator->getCurrentFloor() - request.floor);

        // Prioritize elevators that are:
        // 1. Idle
        // 2. Already moving in the correct direction
        if (elevator->isIdle() ||
            (elevator->isMovingUp() && request.goingUp) ||
            (elevator->isMovingDown() && !request.goingUp)) {

            if (distance < minDistance) {
                minDistance = distance;
                bestElevator = elevator;
            }
        }
    }

    // If no ideal elevator found, assign the closest elevator regardless of state.
    if (!bestElevator) {
        minDistance = INT_MAX;
        for (Elevator* elevator : elevators) {
            int distance = abs(elevator->getCurrentFloor() - request.floor);
            if (distance < minDistance) {
                minDistance = distance;
                bestElevator = elevator;
            }
        }
    }

    // Dispatch the request to the selected elevator.
    if (bestElevator) {
        std::cout << "[Scheduler] Assigned Floor " << request.floor
                  << " to Elevator " << bestElevator->getId() << std::endl;
        bestElevator->addRequest(request);
    } else {
        std::cout << "[Scheduler] No available elevators for Floor " << request.floor << std::endl;
    }
}

/**
 * @brief Processes incoming requests and assigns them to elevators.
 */
void Scheduler::processRequests() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() {
            return !upQueue.empty() || !downQueue.empty() || !elevatorQueue.empty() || isSchedulerStopped;
        });

        //if (isSchedulerStopped && upQueue.empty() && downQueue.empty() && elevatorQueue.empty()) break;
        if (isSchedulerStopped && upQueue.empty() && downQueue.empty() && elevatorQueue.empty()) {
            std::cout << "[Scheduler] No more requests. Exiting..." << std::endl;
            return; // Exit loop when there are no more requests
        }

        Request request;
        bool isFloorRequest = false;

        if (!upQueue.empty()) {
            request = upQueue.front();
            upQueue.pop();
            isFloorRequest = true;
        } else if (!downQueue.empty()) {
            request = downQueue.front();
            downQueue.pop();
            isFloorRequest = true;
        } else if (!elevatorQueue.empty()) {
            request = elevatorQueue.front();
            elevatorQueue.pop();
        }

        lock.unlock(); // Unlock to allow other requests while processing.

        if (isFloorRequest) {
            std::cout << "[Scheduler] Handling FLOOR REQUEST at Floor " << request.floor
                      << " (" << (request.goingUp ? "Up" : "Down") << ")" << std::endl;
            assignElevator(request);
        } else {
            std::cout << "[Scheduler] Handling ELEVATOR REQUEST in Elevator " << request.elevatorId
                      << " to Floor " << request.floor << std::endl;
            assignElevator(request);
        }
    }
}

/**
 * @brief Notifies the Floor subsystem about an elevator arrival.
 * @param floor The floor where the elevator has arrived.
 */
void Scheduler::notifyFloorSubsystem(int floor) {
    std::cout << "[Scheduler] Notifying FloorSubsystem that an elevator has arrived at Floor " << floor << std::endl;
    floorSubsystem->handleElevatorArrival(floor); // Delegate notification to the Floor subsystem.
}

void Scheduler::notifyCompletion(int elevatorId, int floor) {
    std::cout << "[Scheduler] Elevator " << elevatorId << " has completed request at Floor " << floor << std::endl;
}

void Scheduler::setFloorSubsystem(Floor* floorSub){
    floorSubsystem = floorSub;
}

/**
 * @brief Stops the Scheduler and exits the request processing loop.
 */
void Scheduler::stop() {
    std::unique_lock<std::mutex> lock(mtx);
    isSchedulerStopped = true;
    cv.notify_all();
}

