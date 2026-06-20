/**
 * SYSC3303 - Project Iteration 1 
 * Author: Deborah Ajayi
 * Date: February 1st, 2025
 */

#include "Floor.h"
#include <sstream>

/**
 * @brief Constructor for the Floor.
 * @param scheduler Pointer to the Scheduler object.
 */
Floor::Floor(Scheduler* scheduler)
    : scheduler(scheduler), stopSubsystem(false) {}

/**
 * @brief Starts the Floor, processing requests from the file.
 * @param filename The name of the file containing requests to process.
 */
void Floor::start(const std::string& filename) {
    readRequestsFromFile(filename); // Read requests from the specified file
    std::thread requestProcessor(&Floor::processRequests, this);
    requestProcessor.detach(); // Detach the thread to run independently
}

/**
 * @brief Stops the Floor.
 */
void Floor::stop() {
    std::unique_lock<std::mutex> lock(mtx);
    stopSubsystem = true;
    cv.notify_all(); // Notify any waiting threads to exit
}

/**
 * @brief Adds a request to the floor subsystem's queue.
 * @param request The request to be added.
 */
void Floor::addRequestToQueue(const Request& request) {
    std::unique_lock<std::mutex> lock(mtx);
    floorQueue.push(request);
    cv.notify_one(); // Notify the processing thread
}

/**
 * @brief Handles the elevator arrival notification.
 * @param floor The floor where the elevator has arrived.
 */
void Floor::handleElevatorArrival(int floor) {
    std::cout << "[Floor] Elevator has arrived at Floor " << floor << std::endl;

}

/**
 * @brief Processes floor requests and communicates with the Scheduler.
 */
void Floor::processRequests() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() {
            return !floorQueue.empty() || stopSubsystem;
        });

        if (stopSubsystem && floorQueue.empty()) break; // Exit if stopping and no requests

        if (!floorQueue.empty()) {
            Request request = floorQueue.front();
            floorQueue.pop();
            lock.unlock(); // Unlock to allow other requests while processing

            std::cout << "[Floor] Processing request for Floor " << request.floor << std::endl;
            sendToScheduler(request); // Send the request to the Scheduler
        }
    }
}


void Floor::readRequestsFromFile(const std::string& filename) {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "[Floor] Error opening file: " << filename << std::endl;
        return;
    }

    std::cout << "[Floor] Successfully opened: " << filename << std::endl;

    std::string line;
    while (std::getline(inputFile, line)) {
        if (line.empty()) {
            continue;  // Skip empty lines
        }

        std::istringstream iss(line);
        std::string timeStr, direction;
        int floor, carButton;

        // Parse the line
        if (!(iss >> timeStr >> floor >> direction >> carButton)) {
            std::cerr << "[Floor]  Invalid format in line: '" << line << "'" << std::endl;
            continue;
        }

        // Debugging output to ensure the line is parsed correctly
        std::cout << "[Floor]  Parsed Request: Time: " << timeStr
                  << " | Floor: " << floor
                  << " | Direction: " << direction
                  << " | Car Button: " << carButton << std::endl;

        // Determine if the direction is "Up" or "Down" and handle it accordingly
        bool goingUp = (direction == "Up");

        // Create the request and add it to the queue
        Request request(floor, FLOOR_REQUEST, FROM_FLOOR, goingUp);
        addRequestToQueue(request);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Simulate delay
    }

    std::cout << "[Floor]  Finished reading file: " << filename << std::endl;
}

/**
 * @brief Sends a request to the Scheduler.
 * @param request The request to be sent.
 */
void Floor::sendToScheduler(const Request& request) {
    std::cout << "[Floor] Sending request for Floor " << request.floor << " to Scheduler." << std::endl;
    scheduler->addRequest(request.floor, request.type, request.source, request.goingUp);
}

/**
 * @brief Receives a request status update from the Scheduler.
 * @param floor The floor where the elevator has arrived.
 */
void Floor::receiveFromScheduler(int floor) {
    std::cout << "[Floor] Elevator has arrived at Floor " << floor << std::endl;
}

