/**
 * SYSC3303 - Project Iteration 1 
 * Author: Divya Dushyanthan
 * Date: February 1st, 2025
 */

#ifndef ELEVATOR_H
#define ELEVATOR_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include "Scheduler.h"

/**
 * @class Elevator
 * @brief Simulates an elevator handling requests from the Scheduler.
 */
class Elevator {
private:
    int id;                   // Elevator ID
    int currentFloor;         // Tracks the current floor
    bool movingUp;            // Direction of movement (true = up, false = down)
    bool isRunning;           // Flag to indicate if the elevator is active
    std::queue<Request> requestQueue; // Queue of floor requests

    std::mutex mtx;
    std::condition_variable cv;

public:
    /**
     * @brief Constructor for Elevator.
     * @param elevatorId Unique ID of the elevator.
     */
    Elevator(int elevatorId);

    /**
     * @brief Adds a new request to the elevator's queue.
     * @param request The request received from the Scheduler.
     */
    void addRequest(const Request& request);

    /**
     * @brief Gets the current floor of the elevator.
     * @return The current floor number.
     */
    int getCurrentFloor() const;

    /**
     * @brief Checks if the elevator is idle (no pending requests).
     * @return True if idle, false otherwise.
     */
    bool isIdle() const;

    /**
     * @brief Checks if the elevator is moving up.
     * @return True if moving up, false otherwise.
     */
    bool isMovingUp() const;

    /**
     * @brief Checks if the elevator is moving down.
     * @return True if moving down, false otherwise.
     */
    bool isMovingDown() const;

    /**
     * @brief Sends a response to the Scheduler after completing a request.
     * @param scheduler The scheduler to notify.
     * @param floor The floor that was serviced.
     */
    void sendResponseToScheduler(Scheduler* scheduler, int floor);

    /**
     * @brief Main loop for elevator operation.
     * @param scheduler Pointer to the scheduler managing the elevators.
     */
    void run(Scheduler* scheduler);

    /**
     * @brief Stops the elevator system.
     */
    void stop();

    /**
     * @brief Gets the elevator ID.
     * @return The unique elevator ID.
     */
    int getId() const { return id; }
};

#endif // ELEVATOR_H

