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

enum class ElevatorState {
    IDLE,
    MOVING_UP,
    MOVING_DOWN,
    DOOR_OPEN,
    FAULTY,
    OUT_OF_SERVICE
};


class Elevator {
public:
    /**
     * @brief Enum for elevator states.
     */


    enum FaultType {
        DOOR_STUCK = 1,
        FLOOR_TIMER_FAULT = 2
    };


private:
    int id;                                // Elevator ID
    int currentFloor;                      // Current floor
    bool movingUp;                         // Direction (true = up, false = down)
    bool isRunning;                        // Elevator active flag
    int passengerCount;                    // Current load
    std::queue<Request> requestQueue;      // Queue of requests
    std::mutex mtx;
    std::condition_variable cv;
    ElevatorState state;
    DatagramSocket elevatorSocket;         // UDP socket for Scheduler communication

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
     * @brief Simulates the elevator moving.
     * @param Destination floor.
     */
    void simulateMoving(int destinationFloor);

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
     * @brief Periodically sends elevator status (heartbeat) to Scheduler.
     */
    void statusUpdateLoop();


    /**
     * @brief Gets the elevator ID.
     * @return The unique elevator ID.
     */
    int getId() const { return id; }

    /**
     * @brief Gets the current load of the elevator.
     * @return Passenger count.
     */
    int getLoad() const { return passengerCount; }

    void setCurrentFloor(int floor) { currentFloor = floor; }
    
    void sendFaultToScheduler();
    void setState(ElevatorState newState) { state = newState; }
    ElevatorState getState() const { return state; }

    void simulateDoorOperation(int faultCode);

};

#endif // ELEVATOR_H
