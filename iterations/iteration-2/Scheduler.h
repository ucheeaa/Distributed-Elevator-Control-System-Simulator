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

class Elevator;
class Floor;
/**
 * Enum for different request types.
 */
enum RequestType {
    FLOOR_REQUEST,    // User on a floor presses a button
    ELEVATOR_REQUEST, // User inside an elevator selects a floor
    ELEVATOR_REPORT   // Elevator reports reaching a floor
};

/**
 * Enum for identifying the source of a request.
 */
enum RequestSource {
    FROM_FLOOR,   // Request originated from a floor button
    FROM_ELEVATOR // Request originated from inside the elevator
};

/**
 * Struct to represent a request.
 */
struct Request {
    int floor;            // Target floor for the request
    int destinationFloor;
    RequestType type;     // The type of request (Floor, Elevator, Report)
    RequestSource source; // Where the request came from
    bool goingUp;         // True if it's an "Up" request, False if "Down" (for Floor Requests)
    int elevatorId;       // Only relevant for ELEVATOR_REQUEST and ELEVATOR_REPORT

    // Parameterized constructor
    Request(int f, int d, RequestType t, RequestSource s, bool up = true, int e = -1)
            : floor(f), destinationFloor(d), type(t), source(s), goingUp(up), elevatorId(e) {}

    // Default constructor
    Request() : floor(0), destinationFloor(0), type(RequestType::FLOOR_REQUEST), source(RequestSource::FROM_FLOOR), goingUp(true), elevatorId(-1) {}
};

/**
 * Scheduler class that manages elevator requests.
 */
class Scheduler {
public:
    /**
     * @brief Enum for scheduler states
     */
    enum SchedulerState {
        IDLE,         // waiting for new request
        PROCESSING,   // handling new request (selecting elevator)
        WAITING,      // waiting for elevator response
        UPDATING,     // notifying Floor
        SHUTTING_DOWN // system shutdown
    };

private:
    std::queue<Request> upQueue;   // Queue for "Up" floor requests
    std::queue<Request> downQueue; // Queue for "Down" floor requests
    std::queue<Request> elevatorQueue; // Queue for requests from inside elevators

    std::vector<Elevator*> elevators; // List of elevators
    Floor* floorSubsystem;            // Pointer to the Floor subsystem for communication
    std::mutex mtx;                   // Mutex for thread safety
    std::condition_variable cv;       // Condition variable for synchronization

    SchedulerState state;

public:
    /**
     * Constructor to initialize the scheduler with elevators.
     * @param elevators A vector of pointers to Elevator objects.
     * @param floorSub Pointer to the Floor subsystem.
     */
    Scheduler(std::vector<Elevator*>& elevators, Floor* floorSub);

    /**
     * Adds a request to the scheduler queue.
     * @param floor The floor the request is for.
     * @param type The type of request.
     * @param source The origin of the request.
     * @param goingUp True if request is to go up, False if going down.
     * @param elevatorId The ID of the elevator (only relevant for ELEVATOR_REQUEST or REPORT).
     */
    void addRequest(int floor, int destinationFloor, RequestType type, RequestSource source, bool goingUp = true, int elevatorId = -1);

    /**
     * Processes all pending requests by dispatching them to the appropriate elevators.
     */
    void processRequests();

    /**
     * Assigns an elevator to handle a request.
     * @param request The request to assign.
     */
    void assignElevator(const Request& request);

    /**
     * Notifies the Floor subsystem about elevator arrival.
     * @param floor The floor at which the elevator has arrived.
     */
    void notifyFloorSubsystem(int floor);

    void setFloorSubsystem(Floor* floorSub);


    /**
     * Stops the scheduler and releases any pending tasks.
     */
    void stop();

    /**
     * @brief Notifies the Scheduler when an elevator has completed a request.
     * @param elevatorId The ID of the elevator that completed the request.
     * @param floor The floor where the request was completed.
     */
    void notifyCompletion(int elevatorId, int floor, int destinationFloor);

};

#endif // SCHEDULER_H
