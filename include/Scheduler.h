#ifndef SCHEDULER_H
#define SCHEDULER_H

/**
 * SYSC3303 - Project Iteration 5
 * Group: B2-G5
 * Date: April 7th, 2025
 */

#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <map>
#include <chrono>
#include "Datagram.h"

// Forward declarations
class Floor;
class Elevator;
enum class ElevatorState;

/**
 * @enum RequestType
 * @brief Identifies whether the request originated from a floor or an elevator.
 */
enum RequestType {
    FLOOR_REQUEST,
    ELEVATOR_REQUEST
};

/**
 * @enum StatusType
 * @brief Status codes sent between elevator and scheduler.
 */
enum StatusType {
    STATUS_UPDATE = 1,
    STATUS_COMPLETE = 2,
    STATUS_HARD_FAULT = 3,
    STATUS_TRANSIENT_FAULT = 4
};

/**
 * @enum RequestSource
 * @brief Specifies if the request came from the floor or elevator system.
 */
enum RequestSource {
    FROM_FLOOR,
    FROM_ELEVATOR
};

/**
 * @struct Request
 * @brief Represents a pickup/destination request between floor and elevator.
 */
struct Request {
    int floor;
    int destinationFloor;
    bool goingUp;
    int elevatorId;
    int faultFlag;             ///< 0 = no fault, 1 or 2 = fault injected
    int timestamp_ms;
    RequestType type;
    RequestSource source;

    Request(int f, int d, RequestType t, RequestSource s, bool up = true, int e = -1, int ts = 0, int fault = 0)
            : floor(f), destinationFloor(d), type(t), source(s), goingUp(up), elevatorId(e), timestamp_ms(ts), faultFlag(fault) {}

    Request()
            : floor(0), destinationFloor(0), faultFlag(0), type(FLOOR_REQUEST), source(FROM_FLOOR),
              goingUp(true), elevatorId(-1), timestamp_ms(0) {}
};

/**
 * @struct ElevatorStatus
 * @brief Holds current state and floor for an elevator.
 */
struct ElevatorStatus {
    int currentFloor;
    ElevatorState state;
};

extern std::map<int, ElevatorStatus> elevatorStatuses;

/**
 * @class Scheduler
 * @brief Controls and assigns elevator operations based on system events.
 */
class Scheduler {
public:
    /**
     * @enum SchedulerState
     * @brief Tracks the processing state of the Scheduler.
     */
    enum SchedulerState {
        IDLE,
        PROCESSING,
        SHUTTING_DOWN
    };

private:
    // Request queues
    std::queue<Request> upQueue;
    std::queue<Request> downQueue;
    std::queue<Request> elevatorQueue;

    // System state
    SchedulerState state;
    std::mutex mtx;
    std::condition_variable cv;

    // System components
    std::vector<Elevator*> elevators;
    Floor* floorSubsystem;

    // Elevator info and health tracking
    std::unordered_map<int, int> elevatorFloors;
    std::unordered_map<int, std::queue<Request>> elevatorRequestQueue;
    std::unordered_map<int, bool> elevatorDirections;
    std::unordered_map<int, std::chrono::steady_clock::time_point> elevatorLastUpdate;
    std::unordered_map<int, bool> elevatorFaults;
    std::unordered_map<int, ElevatorState> elevatorStates;

    std::unordered_map<int, int> previousFloor;
    std::map<int, int> pickupFloor;

    // UDP communication
    DatagramSocket socket;
    DatagramSocket* socketPtr; // Optional test socket override

public:
    // ────────────────────────────── Constructors ──────────────────────────────
    Scheduler();
    explicit Scheduler(DatagramSocket* customSocket);  ///< Inject custom socket (for testing)

    // ─────────────────────────────── Core Methods ──────────────────────────────
    void addRequest(int floor, int destinationFloor, RequestType type, RequestSource source, bool goingUp = true, int elevatorId = -1, int faultFlag = 0);
    void processRequests();
    void receiveElevatorUpdates();
    void listenForRequests();
    void listenForElevatorRequests();
    void assignElevator(const Request& request);
    void notifyFloor(int elevatorId, int originFloor, int pickupFloor, int destinationFloor);
    void stop();
    void checkForStuckElevators();
    void updateElevatorStatus(int elevatorId, int floor);
    void handleFault(int elevatorId);
    void broadcastElevatorStatus();
    void run();
    void displayStatusLoop();


    // ─────────────────────────────── Getters (for Testing) ──────────────────────────────
    std::queue<Request>& getUpQueue() { return upQueue; }
    std::queue<Request>& getDownQueue() { return downQueue; }
    std::unordered_map<int, int>& getElevatorFloors() { return elevatorFloors; }
    std::unordered_map<int, bool>& getElevatorFaults() { return elevatorFaults; }
    std::unordered_map<int, std::queue<Request>>& getElevatorRequestQueue() { return elevatorRequestQueue; }
    std::unordered_map<int, std::chrono::steady_clock::time_point>& getElevatorLastUpdate() { return elevatorLastUpdate; }

#ifdef TEST_MODE
    DatagramSocket*& getSocketPtr() { return socketPtr; }

// ─────────────── Testing Variables ───────────────
private:
int completedRequestCount = 0;
int retryCount = 0;
bool transientFaultRecovered = false;
int hardFaultElevator = -1;
int droppedRequestCount = 0;

// ─────────────── Testing Helpers ───────────────
public:
void markRequestCompleted() { completedRequestCount++; }
int getCompletedRequestCount() const { return completedRequestCount; }

void incrementRetryCount() { retryCount++; }
int getRetryRequestCount() const { return retryCount; }

void setTransientFaultRecovered() { transientFaultRecovered = true; }
bool wasTransientFaultHandled() const { return transientFaultRecovered; }

void markElevatorFaulty(int id) {
    elevatorFaults[id] = true;
    hardFaultElevator = id;
}
bool isElevatorFaulty(int id) const {
    auto it = elevatorFaults.find(id);
    return it != elevatorFaults.end() && it->second;
}
int getHardFaultElevatorId() const { return hardFaultElevator; }

//void incrementDroppedCount() { droppedRequestCount++; }

//int getDroppedRequestCount() const { return droppedRequestCount; }

#endif
};

#endif // SCHEDULER_H