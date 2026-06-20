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
#include "Datagram.h"

/**
 * @class Elevator
 * @brief Simulates an elevator handling requests from the Scheduler.
 */
class Elevator {

public:
    /**
     * @brief Enum for elevator states.
     */
    enum class ElevatorState {
        IDLE,
        MOVING_UP,
        MOVING_DOWN,
        DOOR_OPEN
    };

private:
    int id;                      // Elevator ID
    int currentFloor;            // Tracks the current floor
    bool movingUp;               // Direction of movement (true = up, false = down)
    bool isRunning;              // Flag to indicate if the elevator is active
    int passengerCount;
    std::queue<Request> requestQueue;   // Queue of floor requests
    std::mutex mtx;
    std::condition_variable cv;
    ElevatorState state;
    DatagramSocket elevatorSocket;      // UDP socket for communication with the Scheduler

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
     * @param floor The floor that was serviced.
     * @param destinationFloor The final destination.
     */
    void sendResponseToScheduler(int floor, int destinationFloor);

    /**
     * @brief Main loop for elevator operation.
     */
    void run();

    /**
     * @brief Listens for requests from the Scheduler via UDP.
     */
    void listenForRequests();

    /**
     * @brief Stops the elevator system.
     */
    void stop();

    /**
     * @brief Sends a periodic status update of the elevator’s floor.
     */
    void sendElevatorStatus();

    /**
     * @brief Gets the elevator ID.
     * @return The unique elevator ID.
     */
    int getId() const { return id; }

    // Get the current load of the elevator (number of passengers)
    int getLoad() const { return passengerCount; }
};

#endif // ELEVATOR_H
