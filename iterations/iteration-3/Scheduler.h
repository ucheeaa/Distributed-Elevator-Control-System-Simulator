/**
 * SYSC3303 - Project Iteration 1 
 * Author: Uchenna Obikwelu
 * Date: February 1st, 2025
 */

#ifndef SCHEDULER_H  
#define SCHEDULER_H  

#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <limits>
#include <unordered_map>
#include "Datagram.h"

// Forward declarations (global scope)
class Floor;
class Elevator;

/**
 * Enum for different request types.
 */
enum RequestType {
    FLOOR_REQUEST,
    ELEVATOR_REQUEST,
    ELEVATOR_REPORT
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
    RequestType type;
    RequestSource source;
    bool goingUp;
    int elevatorId;

    Request(int f, int d, RequestType t, RequestSource s, bool up = true, int e = -1)
            : floor(f), destinationFloor(d), type(t), source(s), goingUp(up), elevatorId(e) {}

    Request() : floor(0), destinationFloor(0), type(FLOOR_REQUEST), source(FROM_FLOOR), goingUp(true), elevatorId(-1) {}
};

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
    std::unordered_map<int, bool> elevatorDirections; // Tracks current elevator directions (true = up, false = down)

    SchedulerState state;
    DatagramSocket socket;

public:
    Scheduler();

    void addRequest(int floor, int destinationFloor, RequestType type, RequestSource source, bool goingUp = true, int elevatorId = -1);
    void processRequests();
    void receiveElevatorUpdates();
    int requestElevatorFloor(int elevatorId);
    void listenForRequests();
    void assignElevator(const Request& request);
    void notifyFloor(int floor);
    void notifyCompletion(int elevatorId, int floor, int destinationFloor);
    void stop();
};

#endif // SCHEDULER_H