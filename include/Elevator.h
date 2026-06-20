#ifndef ELEVATOR_H
#define ELEVATOR_H

/**
 * SYSC3303 - Project Iteration 5
 * Group: B2-G5
 * Date: April 7th, 2025
 */

#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include "Scheduler.h"
#include "Datagram.h"

/**
 * @enum ElevatorState
 * @brief Represents the current state of the elevator.
 */
enum class ElevatorState {
    IDLE,
    MOVING_UP,
    MOVING_DOWN,
    DOOR_OPEN,
    FAULTY,
    OUT_OF_SERVICE
};

/**
 * @class Elevator
 * @brief Simulates an elevator that receives requests from the Scheduler and processes them.
 *        Handles capacity, direction, fault handling, and communication with the Scheduler.
 */
class Elevator {
public:
    /**
     * @enum FaultType
     * @brief Represents fault types that can be injected into an elevator.
     */
    enum FaultType {
        DOOR_STUCK = 1,         ///< Transient fault: elevator door stuck
        FLOOR_TIMER_FAULT = 2   ///< Hard fault: elevator fails to reach floor (timeout)
    };

private:
    int id;                                     ///< Unique elevator ID
    int currentFloor;                           ///< Current floor of the elevator
    bool movingUp;                              ///< Direction: true = up, false = down
    bool isRunning;                             ///< Whether the elevator is actively running
    int passengerCount;                         ///< Current number of passengers
    ElevatorState state;                        ///< Current state of the elevator

    std::queue<Request> requestQueue;           ///< Queue of pending requests
    std::vector<Request> activePassengers;      ///< Passengers currently on board
    std::vector<Request> requestsAtThisFloor;   ///< Requests to be picked up at current floor

    std::mutex mtx;                             ///< Mutex for thread safety
    std::condition_variable cv;                 ///< Condition variable to manage thread waiting

    DatagramSocket elevatorSocket;              ///< UDP socket for communication with Scheduler
    std::chrono::steady_clock::time_point startTime; ///< Timestamp for movement duration
    int totalMovements = 0;                     ///< Count of total floor movements
    bool firstPassComplete = false;             ///< Used to manage first pickup behavior

public:
    // ─────────────────────────────────────────────────────
    // Constructor
    // ─────────────────────────────────────────────────────

    /**
     * @brief Constructor.
     * @param elevatorId ID to uniquely identify this elevator.
     */
    Elevator(int elevatorId);

    // ─────────────────────────────────────────────────────
    // Core Behavior
    // ─────────────────────────────────────────────────────

    /**
     * @brief Main loop for elevator operation.
     */
    void run();

    /**
     * @brief Stops the elevator gracefully.
     */
    void stop();

    /**
     * @brief Receives elevator requests from the Scheduler.
     */
    void listenForRequests();

    /**
     * @brief Periodically sends elevator status (heartbeat) to Scheduler.
     */
    void statusUpdateLoop();

    // ─────────────────────────────────────────────────────
    // Request Handling
    // ─────────────────────────────────────────────────────

    /**
     * @brief Adds a new request to the elevator's queue.
     * @param request The request to add.
     */
    void addRequest(const Request& request);

    /**
     * @brief Handles boarding of passengers at a specific floor, respecting capacity limits.
     * @param currentPickupFloor Floor where passengers are waiting.
     */
    void handlePickupFloor(int currentPickupFloor);

    /**
     * @brief Resends a request to Scheduler when elevator is full or fault occurs.
     * @param originalRequest The request to be requeued.
     */
    void resendRequestToScheduler(const Request& originalRequest);

    /**
     * @brief Simulates elevator moving to a specific floor.
     * @param destinationFloor The floor to move to.
     */
    void simulateMoving(int destinationFloor);

    /**
     * @brief Simulates door open/close sequence, with optional fault injection.
     * @param faultCode 1 for door fault simulation, 0 for normal.
     */
    void simulateDoorOperation(int faultCode);

    /**
     * @brief Sends a "trip completed" update to the Scheduler.
     * @param origin Start floor.
     * @param pickupFloor Pickup floor.
     * @param destinationFloor Drop-off floor.
     */
    void sendResponseToScheduler(int origin, int pickupFloor, int destinationFloor);

    /**
     * @brief Sends an elevator state update to the Scheduler (heartbeat).
     */
    void sendElevatorStatus();

    /**
     * @brief Sends a fault report to the Scheduler (transient or hard).
     * @param faultType The type of fault (from StatusType).
     */
    void sendFaultToScheduler(StatusType faultType);

    // ─────────────────────────────────────────────────────
    // Getters and Setters
    // ─────────────────────────────────────────────────────

    /**
     * @brief Returns current floor.
     */
    int getCurrentFloor() const;

    /**
     * @brief Sets current floor (used in test/debug).
     */
    void setCurrentFloor(int floor);

    /**
     * @brief Returns elevator ID.
     */
    int getId() const;

    /**
     * @brief Returns passenger count.
     */
    int getPassengerCount() const;

    /**
     * @brief Returns elevator load (same as passenger count).
     */
    int getLoad() const;

    /**
     * @brief Returns true if elevator is currently idle.
     */
    bool isIdle() const;

    /**
     * @brief Returns true if elevator is moving up.
     */
    bool isMovingUp() const;

    /**
     * @brief Returns true if elevator is moving down.
     */
    bool isMovingDown() const;

    /**
     * @brief Sets the elevator state.
     * @param newState The state to assign.
     */
    void setState(ElevatorState newState);

    /**
     * @brief Gets the current state of the elevator.
     */
    ElevatorState getState() const;
};

#endif // ELEVATOR_H
