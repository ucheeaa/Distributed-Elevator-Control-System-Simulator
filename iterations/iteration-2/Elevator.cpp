/**
 * SYSC3303 - Project Iteration 1 
 * Author: Divya Dushyanthan
 * Date: February 1st, 2025
 */

#include "Elevator.h"        
#include <iostream>           
#include <thread>             

/**
 * @brief Constructor for the Elevator class.
 * @param elevatorId Unique identifier for the elevator.
 */
Elevator::Elevator(int elevatorId)
    : id(elevatorId), currentFloor(0), movingUp(true), isRunning(true), state(ElevatorState::IDLE) {}

/**
 * @brief Adds a request to the elevator's queue.
 * @param request The request to be processed.
 * 
 * This function locks the mutex to ensure thread-safe access
 * to the request queue, adds the new request, and notifies
 * the elevator thread that a new request is available.
 */
void Elevator::addRequest(const Request& request) {
    std::unique_lock<std::mutex> lock(mtx); // Ensure thread safety
    requestQueue.push(request);
    cv.notify_one(); // Notify the elevator thread that a new request is available
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
 * @brief Sends a response to the scheduler upon reaching the requested floor.
 * @param scheduler Pointer to the scheduler managing the elevators.
 * @param floor The floor the elevator has reached.
 * 
 * This function informs the scheduler that the elevator has 
 * successfully completed a request by reaching the target floor.
 */
void Elevator::sendResponseToScheduler(Scheduler* scheduler, int floor, int destinationFloor) {
    if (scheduler) {
        scheduler->notifyCompletion(id, floor, destinationFloor);
    }
}

/**
 * @brief Main loop for elevator operation.
 * @param scheduler Pointer to the scheduler managing the elevators.
 * 
 * The elevator continuously checks for new requests.
 * If the queue is empty, it waits until a new request is received.
 * When a request is found, it processes it by moving to the requested floor.
 */
void Elevator::run(Scheduler* scheduler) {
    while (isRunning) {
        std::unique_lock<std::mutex> lock(mtx);
        
        // Wait until there is a request or the system is shutting down
        cv.wait(lock, [this] { return !requestQueue.empty() || !isRunning; });

        // If the elevator is being stopped, exit the thread
        if (!isRunning) {
            std::cout << "[Elevator " << id << "] Stopping and exiting thread.\n";
            return;
        }

        // Prevent unnecessary loops if queue is empty
        if (requestQueue.empty()) {
            continue;
        }

        // Retrieve the next request from the queue
        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock(); // Unlock before processing the request

        std::cout << "[Elevator] Elevator " << id << " moving to handle request at floor " << req.floor<< std::endl;

        // Transition to MOVING state
        if (req.goingUp) {
            state = Elevator::ElevatorState::MOVING_UP;
            std::cout << "[Elevator] Elevator " << id << " moving UP to floor " << req.destinationFloor << std::endl;
        } else if (!req.goingUp) {
            state = Elevator::ElevatorState::MOVING_DOWN;
            std::cout << "[Elevator] Elevator " << id << " moving DOWN to floor " << req.destinationFloor << std::endl;
        }
        
        currentFloor = req.floor; // Update elevator's current floor
        std::cout << "[Elevator] Elevator " << id << " reached floor " << req.destinationFloor << std::endl;
        
        // Transition to DOOR_OPEN state
        state = Elevator::ElevatorState::DOOR_OPEN;
        std::cout << "[Elevator] Elevator " << id << " doors opening..." << std::endl;
        
        // Transition to DOOR_CLOSED state
        std::cout << "[Elevator] Elevator " << id << " doors closing..." << std::endl;
        
        // Return to IDLE state
        state = Elevator::ElevatorState::IDLE;

        // Notify the scheduler that the request has been completed
        sendResponseToScheduler(scheduler, currentFloor, req.destinationFloor);
    }
}

/**
 * @brief Stops the elevator operation.
 * 
 * This function sets the `isRunning` flag to false and 
 * notifies the condition variable to ensure the thread exits.
 */
void Elevator::stop() {
    std::unique_lock<std::mutex> lock(mtx);
    isRunning = false;
    cv.notify_all();  // Notify any waiting threads to exit
}
