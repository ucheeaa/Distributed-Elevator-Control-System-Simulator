/**
 * SYSC3303 - Project Iteration 1 
 * Author: Divya Dushyanthan
 * Date: February 1st, 2025
 */

#include "Elevator.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

/**
 * @brief Constructor for the Elevator class.
 * @param elevatorId Unique identifier for the elevator.
 */
Elevator::Elevator(int elevatorId)
    : id(elevatorId), currentFloor(0), movingUp(true), isRunning(true) {}

/**
 * @brief Adds a request to the elevator's queue.
 * @param request The request to be processed.
 */
void Elevator::addRequest(const Request& request) {
    std::unique_lock<std::mutex> lock(mtx);
    requestQueue.push(request);
    cv.notify_one(); // Notify the elevator thread of a new request
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
    return requestQueue.empty();
}

/**
 * @brief Checks if the elevator is moving up.
 * @return True if the elevator is moving up, false otherwise.
 */
bool Elevator::isMovingUp() const {
    return movingUp;
}

/**
 * @brief Checks if the elevator is moving down.
 * @return True if the elevator is moving down, false otherwise.
 */
bool Elevator::isMovingDown() const {
    return !movingUp;
}

/**
 * @brief Sends a response to the scheduler upon reaching the requested floor.
 * @param scheduler Pointer to the scheduler managing the elevators.
 * @param floor The floor the elevator has reached.
 */
void Elevator::sendResponseToScheduler(Scheduler* scheduler, int floor) {
    if (scheduler) {
        scheduler->notifyCompletion(id, floor);
    }
}

/**
 * @brief Main loop for elevator operation.
 * @param scheduler Pointer to the scheduler managing the elevators.
 */
void Elevator::run(Scheduler* scheduler) {
    while (isRunning) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !requestQueue.empty() || !isRunning; });
        
        if (!isRunning) break;

        if (requestQueue.empty()) {
            std::cout << "[Elevator " << id << "] No more requests. Exiting...\n";
            return;
        }

        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock();
        
        std::cout << "[Elevator] Elevator " << id << " moving to floor " << req.floor << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Simulating travel time
        
        currentFloor = req.floor;
        std::cout << "[Elevator] Elevator " << id << " reached floor " << currentFloor << std::endl;
        
        sendResponseToScheduler(scheduler, currentFloor);
    }
}

/**
 * @brief Stops the elevator operation and notifies all waiting threads.
 */
void Elevator::stop() {
    std::unique_lock<std::mutex> lock(mtx);
    isRunning = false;
    cv.notify_all();
}
