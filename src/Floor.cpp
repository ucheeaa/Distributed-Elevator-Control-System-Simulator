/**
 * @file Floor.cpp
 * @brief Handles the Floor subsystem that reads and processes floor requests and communicates with the Scheduler.
 *
 * SYSC3303 - Project Iteration 5
 * Group: B2-G5
 * Date: April 7th, 2025
 */

#include "Floor.h"
#include <sstream>
#include <algorithm>
#include <iostream>
#include <thread>

#define SCHEDULER_PORT 50000        ///< Port where Scheduler listens for requests
#define FLOOR_LISTEN_PORT 50002     ///< Port where Floor listens for elevator notifications
#define MAX_FLOORS 22               ///< Maximum number of floors in the system

/**
 * @brief Default constructor for Floor subsystem.
 */
Floor::Floor() : stopSubsystem(false) {}

/**
 * @brief Starts the Floor system by reading input requests and processing them.
 * @param filename Path to input file containing floor requests.
 */
void Floor::start(const std::string& filename) {
    readRequestsFromFile(filename);   // Load and sort requests
    processRequests();                // Begin processing them
}

/**
 * @brief Adds a new request to the request queue (thread-safe).
 * @param request A valid Request object representing a floor button press.
 */
void Floor::addRequestToQueue(const Request& request) {
    std::unique_lock<std::mutex> lock(mtx);
    floorQueue.push(request);
    cv.notify_one();  // Wake processing thread
}

/**
 * @brief Converts a timestamp string ("HH:MM:SS.mmm") into milliseconds since 00:00:00.000.
 * @param timeStr The formatted time string.
 * @return Time in milliseconds.
 */
int convertTimeToMs(const std::string& timeStr) {
    int hours, minutes, seconds, millis;
    char colon1, colon2, dot;
    std::istringstream ss(timeStr);
    ss >> hours >> colon1 >> minutes >> colon2 >> seconds >> dot >> millis;
    return ((hours * 3600) + (minutes * 60) + seconds) * 1000 + millis;
}

/**
 * @brief Continuously processes floor requests and sends them to the Scheduler at the correct simulated time.
 */
void Floor::processRequests() {
    auto simulationStart = std::chrono::steady_clock::now();  // Simulation reference time

    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() {
            return !floorQueue.empty() || stopSubsystem;
        });

        if (stopSubsystem && floorQueue.empty()) {
            std::cout << "[Floor] No more requests. Stopping..." << std::endl;
            return;
        }

        if (!floorQueue.empty()) {
            Request request = floorQueue.front();
            floorQueue.pop();
            lock.unlock();

            if (request.floor < 1 || request.floor > MAX_FLOORS) {
                std::cerr << "[Floor] ERROR: Invalid floor number: " << request.floor << std::endl;
                continue;
            }

            int elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - simulationStart).count();

            if (elapsed_ms < request.timestamp_ms) {
                int wait_time = request.timestamp_ms - elapsed_ms;
                std::cout << "[Floor] Waiting " << wait_time << " ms before sending request for Floor "
                          << request.floor << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
            }

            std::cout << "[Floor] Processing request for Floor " << request.floor << std::endl;
            sendToScheduler(request);
        }
    }
}

/**
 * @brief Stops the Floor subsystem safely (e.g., during shutdown or unit tests).
 */
void Floor::stop() {
    std::unique_lock<std::mutex> lock(mtx);
    stopSubsystem = true;
    cv.notify_all();
}

/**
 * @brief Reads and parses floor requests from a file, converting them to Request objects.
 * @param filename Input file containing floor request lines.
 */
void Floor::readRequestsFromFile(const std::string& filename) {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "[Floor] Error opening file: " << filename << std::endl;
        return;
    }

    std::cout << "[Floor] Successfully opened: " << filename << std::endl;

    std::string line;
    int baseTimestamp = -1;
    std::vector<Request> requests;

    while (std::getline(inputFile, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string timeStr, direction;
        int floorNum = -1, carButton = -1, fault = 0;

        if (!(iss >> timeStr >> floorNum >> direction >> carButton >> fault)) {
            std::cerr << "[Floor] ERROR: Invalid line format: '" << line << "'" << std::endl;
            continue;
        }

        if (floorNum < 1 || floorNum > MAX_FLOORS) {
            std::cerr << "[Floor] ERROR: Invalid floor number parsed: " << floorNum << std::endl;
            continue;
        }

        bool goingUp = (direction == "Up");
        int timestamp = convertTimeToMs(timeStr);

        if (baseTimestamp == -1) baseTimestamp = timestamp;
        timestamp -= baseTimestamp;

        Request request;
        request.floor = floorNum;
        request.destinationFloor = carButton;
        request.goingUp = goingUp;
        request.faultFlag = fault;
        request.timestamp_ms = timestamp;
        request.type = FLOOR_REQUEST;
        request.source = FROM_FLOOR;

        requests.push_back(request);
    }

    std::stable_sort(requests.begin(), requests.end(), [](const Request& a, const Request& b) {
        return a.timestamp_ms < b.timestamp_ms;
    });

    for (const auto& req : requests) {
        std::cout << "[Floor] Sorted Request: Time = " << req.timestamp_ms
                  << " ms, Floor = " << req.floor
                  << ", Dest = " << req.destinationFloor
                  << ", Fault = " << req.faultFlag << std::endl;
        addRequestToQueue(req);
    }

    std::cout << "[Floor] Finished reading and sorting file: " << filename << std::endl;
}

/**
 * @brief Sends a request to the Scheduler via UDP.
 * @param request The floor request to send.
 */
void Floor::sendToScheduler(const Request& request) {
    std::cout << "[Floor] Sending request for Floor " << request.floor
              << " (Direction: " << (request.goingUp ? "Up" : "Down") << ") to Scheduler." << std::endl;

    DatagramSocket socket;
    std::vector<uint8_t> data(sizeof(Request));
    memcpy(data.data(), &request, sizeof(Request));
    DatagramPacket packet(data, sizeof(Request), InetAddress::getLocalHost(), SCHEDULER_PORT);
    socket.send(packet);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // small delay to avoid flooding
}

/**
 * @brief Listens for elevator arrival notifications from the Scheduler.
 */
void Floor::receiveFromScheduler() {
    DatagramSocket socket(FLOOR_LISTEN_PORT);

    while (true) {
        std::vector<uint8_t> in(sizeof(int) * 4);
        DatagramPacket receivePacket(in, in.size());

        try {
            socket.receive(receivePacket);
        } catch (const std::runtime_error &e) {
            std::cerr << "[Floor] Error receiving from Scheduler: " << e.what() << std::endl;
            continue;
        }

        int elevatorId, originFloor, pickupFloor, destinationFloor;
        memcpy(&elevatorId, in.data(), sizeof(int));
        memcpy(&originFloor, in.data() + sizeof(int), sizeof(int));
        memcpy(&pickupFloor, in.data() + 2 * sizeof(int), sizeof(int));
        memcpy(&destinationFloor, in.data() + 3 * sizeof(int), sizeof(int));

        std::cout << "[Floor] Elevator " << elevatorId
                  << " came from Floor " << originFloor
                  << ", picked up at Floor " << pickupFloor
                  << ", and arrived at Floor " << destinationFloor << std::endl;
    }
}

/**
 * @brief Returns the size of the current floor request queue (for testing).
 * @return Number of pending floor requests.
 */
size_t Floor::getQueueSize() {
    std::unique_lock<std::mutex> lock(mtx);
    return floorQueue.size();
}

//For integration Testing

void Floor::run() {
    std::thread listenerThread(&Floor::receiveFromScheduler, this);
    this->start("integration_test_input.txt");  // Use your integration test input file here
    listenerThread.join();
}



#ifndef TEST_MODE
/**
 * @brief Main entry point for standalone Floor testing.
 */
int main() {
    Floor floor;
    std::thread listenerThread(&Floor::receiveFromScheduler, &floor);
    listenerThread.detach();
    floor.start("input.txt");
    return 0;
}
#endif
