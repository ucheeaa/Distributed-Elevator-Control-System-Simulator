/**
 * SYSC3303 - Project Iteration 1 
 * Author: Uchenna Obikwelu
 * Date: February 1st, 2025
 */

#include <iostream>    
#include <thread>      
#include <climits>     
#include <optional>   
#include <mutex>       

#include "Scheduler.h"
#include "Elevator.h" 
#include "Floor.h"     

/**
 * @brief Constructor for the Scheduler.
 * @param elevators A vector of pointers to Elevator objects.
 * @param floorSubsystem A pointer to the Floor subsystem.
 */
Scheduler::Scheduler(std::vector<Elevator*>& elevators, Floor* floorSubsystem)
        : elevators(elevators), floorSubsystem(floorSubsystem), state(SchedulerState::IDLE) {}

/**
 * @brief Adds a new request to the scheduler's queue.
 * @param floor The target floor for the request.
 * @param type The type of request (Floor/Elevator).
 * @param source The source of the request (From Floor/Elevator).
 * @param goingUp True for "Up" requests, False for "Down".
 * @param elevatorId The ID of the elevator (relevant for elevator requests).
 */
void Scheduler::addRequest(int floor, int destinationFloor, RequestType type, RequestSource source, bool goingUp, int elevatorId) {
    std::unique_lock<std::mutex> lock(mtx);  // Lock the mutex for thread safety
    
    Request request(floor, destinationFloor, type, source, goingUp, elevatorId);

    // Add request to the appropriate queue based on direction
    if (type == FLOOR_REQUEST) {
        if (goingUp) {
            upQueue.push(request);
        } else {
            downQueue.push(request);
        }
        cv.notify_one();  // Notify processing thread that a new request is available
    } 

    state = Scheduler::SchedulerState::PROCESSING; // request received, transition to processing
    std::cout << "[Scheduler] Received FLOOR REQUEST at Floor " << floor
              << " (Direction: " << (goingUp ? "Up" : "Down") << ")" << std::endl;

    // Ensure this message appears before handling starts
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

/**
 * @brief Assigns the best elevator to handle the given request.
 * @param request The request to be assigned.
 */
void Scheduler::assignElevator(const Request& request) {
    Elevator* bestElevator = nullptr;
    int minDistance = INT_MAX;

    // Find the best available elevator based on proximity and direction
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

    // If no ideal elevator is found, assign the closest one regardless of state
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

    // Dispatch the request to the selected elevator
    if (bestElevator) {
        std::cout << "[Scheduler] Assigned Floor " << request.floor
                  << " to Elevator " << bestElevator->getId() << std::endl;
        bestElevator->addRequest(request);
    } else {
        std::cout << "[Scheduler] No available elevators for Floor " << request.floor << std::endl;
    }
    state = Scheduler::SchedulerState::WAITING; //elevator assigned, transition to waiting
}

/**
 * @brief Processes incoming requests and assigns them to elevators.
 */
void Scheduler::processRequests() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() {
            return !upQueue.empty() || !downQueue.empty() || state == Scheduler::SchedulerState::SHUTTING_DOWN;
        });

        // Exit loop when scheduler is stopped and all queues are empty
        if (state == Scheduler::SchedulerState::SHUTTING_DOWN && upQueue.empty() && downQueue.empty()) {
            std::cout << "[Scheduler] No more requests. Exiting..." << std::endl;
            return;
        }

        Request request;
        bool isFloorRequest = false;

        // Process the request from the queue
        if (!upQueue.empty()) {
            request = upQueue.front();
            upQueue.pop();
            isFloorRequest = true;
        } else if (!downQueue.empty()) {
            request = downQueue.front();
            downQueue.pop();
            isFloorRequest = true;
        }

        lock.unlock();  // Unlock mutex before further processing

        if (isFloorRequest) {
            std::cout << "[Scheduler] Handling FLOOR REQUEST at Floor " << request.floor 
                      << " (" << (request.goingUp ? "Up" : "Down") << ")" << std::endl;
            assignElevator(request);
        }
    }
}

/**
 * @brief Stops the scheduler and exits the request processing loop.
 */
void Scheduler::stop() {
    std::unique_lock<std::mutex> lock(mtx);
    state = Scheduler::SchedulerState::SHUTTING_DOWN;
    cv.notify_all();  // Ensure all waiting threads exit
}

/**
 * @brief Notifies the Floor subsystem about an elevator arrival.
 * @param floor The floor where the elevator has arrived.
 */
void Scheduler::notifyFloorSubsystem(int floor) {
    std::cout << "[Scheduler] Notifying FloorSubsystem that an elevator has arrived at Floor " << floor << std::endl;
    floorSubsystem->handleElevatorArrival(floor); // Delegate notification to the Floor subsystem
    state = Scheduler::SchedulerState::UPDATING; // elevator arrived, transition to updating
}

/**
 * @brief Notifies that an elevator has completed a request at a floor.
 * @param elevatorId The ID of the elevator.
 * @param floor The floor where the elevator has stopped.
 */
void Scheduler::notifyCompletion(int elevatorId, int floor, int destinationFloor) {
    std::cout << "[Scheduler] Elevator " << elevatorId << " has completed request for Floor " << floor << " to arrive at Floor " << destinationFloor << std::endl;

    std::unique_lock<std::mutex> lock(mtx);

    //notifyFloorSubsystem(floor);

    state = Scheduler::SchedulerState::IDLE; // Transition state back to idle

    cv.notify_one();

}


/**
 * @brief Sets the Floor subsystem reference in the Scheduler.
 * @param floorSub A pointer to the Floor subsystem.
 */
void Scheduler::setFloorSubsystem(Floor* floorSub) {
    floorSubsystem = floorSub;
}
