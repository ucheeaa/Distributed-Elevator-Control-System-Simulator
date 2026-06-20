/**
 * SYSC3303 - Project Iteration 1 
 * Author: Deborah Ajayi
 * Date: February 1st, 2025
 */

#include "Floor.h"      
#include <sstream>      

/**
 * @brief Constructor for the Floor class.
 * @param scheduler Pointer to the Scheduler object that handles elevator requests.
 */
Floor::Floor(Scheduler* scheduler)
    : scheduler(scheduler), stopSubsystem(false) {}

/**
 * @brief Starts the Floor system, reading requests from a file and processing them.
 * @param filename The name of the file containing requests to process.
 * 
 * This function reads requests from the specified input file, then spawns a separate 
 * thread to continuously process the requests in the queue.
 */
void Floor::start(const std::string& filename) {
    readRequestsFromFile(filename); // Read requests from the specified file

    processRequests();

}

/**
 * @brief Adds a request to the floor subsystem's queue.
 * @param request The request to be added.
 * 
 * This function ensures thread-safe access to the floor queue by locking a mutex.
 * Once the request is added, it notifies the processing thread.
 */
void Floor::addRequestToQueue(const Request& request) {
    std::unique_lock<std::mutex> lock(mtx);
    floorQueue.push(request);
    cv.notify_one(); // Notify the processing thread that a new request is available
}

/**
 * @brief Handles the elevator arrival notification.
 * @param floor The floor where the elevator has arrived.
 * 
 * This function is called when the scheduler notifies that an elevator has reached
 * a floor. It can be expanded to include logic for handling passengers.
 */
void Floor::handleElevatorArrival(int floor) {
    std::cout << "[Floor] Elevator has arrived at Floor " << floor << std::endl;
}

/**
 * @brief Continuously processes floor requests and communicates with the Scheduler.
 * 
 * This function runs in an infinite loop, waiting for new floor requests to process.
 * It stops only when the `stopSubsystem` flag is set and there are no more requests.
 */
void Floor::processRequests() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        
        // Wait for either a new request or a stop signal
        cv.wait(lock, [this]() {
            return !floorQueue.empty() || stopSubsystem;
        });

        // Exit the loop when stopping is requested and the queue is empty
        if (stopSubsystem && floorQueue.empty()) {
            std::cout << "[Floor] No more requests. Stopping..." << std::endl;
            return;
        }

        // Process the next request
        if (!floorQueue.empty()) {
            Request request = floorQueue.front();
            floorQueue.pop();
            lock.unlock(); // Unlock before sending request to the scheduler

            std::cout << "[Floor] Processing request for Floor " << request.floor << std::endl;
            sendToScheduler(request);
        }
    }
}

/**
 * @brief Stops the Floor subsystem.
 * 
 * This function sets the `stopSubsystem` flag to `true` and notifies all 
 * waiting threads to allow them to exit cleanly.
 */
void Floor::stop() {
    std::unique_lock<std::mutex> lock(mtx);
    stopSubsystem = true;
    cv.notify_all();  // Notify all waiting threads to exit
}

/**
 * @brief Reads requests from a file and adds them to the queue.
 * @param filename The name of the input file containing requests.
 * 
 * Each line in the file is expected to follow a specific format:
 * - Time Floor Direction CarButton
 * Example: "10:05 3 Up 5"
 */
void Floor::readRequestsFromFile(const std::string& filename) {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "[Floor] Error opening file: " << filename << std::endl;
        return;
    }

    std::cout << "[Floor] Successfully opened: " << filename << std::endl;

    std::string line;
    while (std::getline(inputFile, line)) {
        if (line.empty()) continue;  // Skip empty lines

        std::istringstream iss(line);
        std::string timeStr, direction;
        int floor = -1, carButton = -1;

        // Parse the input line into components
        if (!(iss >> timeStr >> floor >> direction >> carButton)) {
            std::cerr << "[Floor] ERROR: Invalid format in line: '" << line << "'" << std::endl;
            continue;
        }

        // Validate that the floor number is within the expected range
        if (floor < 1 || floor > 10) {
            std::cerr << "[Floor] ERROR: Invalid floor number parsed: " << floor << std::endl;
            continue;
        }

        // Convert direction string to boolean value
        bool goingUp = (direction == "Up");

        std::cout << "[Floor] Parsed Request: Time: " << timeStr
                  << " | Floor: " << floor
                  << " | Direction: " << (goingUp ? "Up" : "Down")
                  << " | Car Button: " << carButton << std::endl;

        // Create and enqueue a new request
        Request request(floor, carButton, FLOOR_REQUEST, FROM_FLOOR, goingUp);
        addRequestToQueue(request);
    }

    std::cout << "[Floor] Finished reading file: " << filename << std::endl;
}

/**
 * @brief Sends a floor request to the Scheduler.
 * @param request The request to be sent.
 * 
 * This function passes a request to the scheduler for processing.
 * It ensures thread safety by allowing a short delay between log messages.
 */
void Floor::sendToScheduler(const Request& request) {
    std::cout << "[Floor] Sending request for Floor " << request.floor
              << " (Direction: " << (request.goingUp ? "Up" : "Down") << ") to Scheduler." << std::endl;

    scheduler->addRequest(request.floor, request.destinationFloor, request.type, request.source, request.goingUp);

    // Add a short delay to ensure proper log spacing
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    //std::cout << "[Floor] Completed request sent to Scheduler for Floor " << request.floor << std::endl;
}

/**
 * @brief Receives a notification from the Scheduler when an elevator arrives.
 * @param floor The floor where the elevator has arrived.
 */
void Floor::receiveFromScheduler(int floor) {
    std::cout << "[Floor] Elevator has arrived at Floor " << floor << std::endl;
}