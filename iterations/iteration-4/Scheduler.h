/**
 * SYSC3303 - Project Iteration 4
 * Author: Uchenna Obikwelu
 * Date: March 20th, 2025
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <limits>
#include <unordered_map>
#include <map>
#include <chrono>  // Required for time tracking
#include "Datagram.h"



// Forward declarations
class Floor;
class Elevator;
enum class ElevatorState; // Forward declare the enum


/**
 * Enum for different request types.
 */
enum RequestType {
    FLOOR_REQUEST,
    ELEVATOR_REQUEST,
    ELEVATOR_REPORT,
    ELEVATOR_COMPLETE,
    ELEVATOR_FAULT,
    ELEVATOR_STATUS_UPDATE
};

enum StatusType {
    STATUS_UPDATE = 1,       // for heartbeat/status packets
    STATUS_COMPLETE = 2,      // for trip completion packets
    STATUS_FAULT = 3
};

/**
 * Enum for identifying the source of a request.
 */
enum RequestSource {
    FROM_FLOOR,
    FROM_ELEVATOR
};

/**
 * Struct to represent a request.
 */
struct Request {
    int floor;
    int destinationFloor;
    bool goingUp;
    int elevatorId;
    int faultFlag;           // Fault flag (0 = no fault, 1 = fault)
    int timestamp_ms; // <-- Simulated time
    RequestType type;
    RequestSource source;

    Request(int f, int d, RequestType t, RequestSource s, bool up = true, int e = -1, int ts = 0, int fault = 0)
            : floor(f), destinationFloor(d), type(t), source(s), goingUp(up), elevatorId(e), timestamp_ms(ts), faultFlag(fault) {}


    Request()
            : floor(0), destinationFloor(0), faultFlag(0), type(FLOOR_REQUEST), source(FROM_FLOOR), goingUp(true), elevatorId(-1), timestamp_ms(0)
    {}


};

struct ElevatorStatus {
    int currentFloor;
    ElevatorState state;
};

extern std::map<int, ElevatorStatus> elevatorStatuses;

/**
 * Scheduler class that manages elevator requests.
 */
class Scheduler {
public:
    enum SchedulerState {
        IDLE,
        PROCESSING,
        WAITING,
        UPDATING,
        SHUTTING_DOWN
    };


private:
    std::queue<Request> upQueue;
    std::queue<Request> downQueue;
    std::queue<Request> elevatorQueue;

    std::vector<Elevator*> elevators;
    Floor* floorSubsystem;
    std::mutex mtx;
    std::condition_variable cv;
    std::unordered_map<int, int> elevatorFloors;

    std::unordered_map<int, std::queue<Request>> elevatorRequestQueue; // Tracks pending requests per elevator
    std::unordered_map<int, bool> elevatorDirections;                  // Tracks current elevator directions (true = up, false = down)

    std::unordered_map<int, std::chrono::steady_clock::time_point> elevatorLastUpdate; // Last heartbeat per elevator
    std::unordered_map<int, bool> elevatorFaults; // true = faulty, false = working
    std::unordered_map<int, ElevatorState> elevatorStates;

    SchedulerState state;
    DatagramSocket socket;

    //For unit testing
    DatagramSocket* socketPtr;  // Use pointer instead of direct object

public:
    Scheduler();

    // Inject socket (for unit tests)
    Scheduler(DatagramSocket* customSocket);

    void addRequest(int floor, int destinationFloor, RequestType type, RequestSource source, bool goingUp = true, int elevatorId = -1, int faultFlag = 0);

    void processRequests();
    void receiveElevatorUpdates();
    void listenForRequests();
    void assignElevator(const Request& request);
    void notifyFloor(int elevatorId, int originFloor, int pickupFloor, int destinationFloor);
    void stop();
    void checkForStuckElevators();
    void updateElevatorStatus(int elevatorId, int floor);
    void handleFault(int elevatorId);

    // Add these getters to access private members for testing

    std::queue<Request>& getUpQueue() { return upQueue; }
    std::queue<Request>& getDownQueue() { return downQueue; }
    std::unordered_map<int, int>& getElevatorFloors() { return elevatorFloors; }
    std::unordered_map<int, bool>& getElevatorFaults() { return elevatorFaults; }

    // Add this inside your public section:
    std::unordered_map<int, std::queue<Request>>& getElevatorRequestQueue() { return elevatorRequestQueue; }
    std::unordered_map<int, std::chrono::steady_clock::time_point>& getElevatorLastUpdate() { return elevatorLastUpdate; }
    // In Scheduler.h (or wherever your members are)
    std::unordered_map<int, int> previousFloor;

    std::map<int, int> pickupFloor;  // elevatorId -> pickup floor





#ifdef TEST_MODE
    DatagramSocket*& getSocketPtr() { return socketPtr; }
#endif


};

#endif // SCHEDULER_H
