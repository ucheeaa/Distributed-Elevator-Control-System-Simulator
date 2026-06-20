/**
 * SYSC3303 - Project Iteration 3
 * Author: Deborah Ajayi
 * Date: March 8th, 2025
 */

#include "Floor.h"
#include <sstream>   // for std::istringstream

#define SCHEDULER_PORT 50000       // Port where Scheduler listens for requests
#define FLOOR_LISTEN_PORT 50002    // Port where Floor listens for elevator arrivals

/**
 * @brief Constructor for the Floor class.
 */
Floor::Floor() : stopSubsystem(false) {}

/**
 * @brief Starts the Floor system by reading requests from a file and processing them.
 * @param filename The name of the file containing requests to process.
 */
void Floor::start(const std::string& filename) {
    readRequestsFromFile(filename);  // Read requests from the specified file
    processRequests();               // Start processing those requests
}

/**
 * @brief Adds a request to the floor subsystem's queue (thread-safe).
 * @param request The request to be added.
 */
void Floor::addRequestToQueue(const Request& request) {
    std::unique_lock<std::mutex> lock(mtx);
    floorQueue.push(request);
    cv.notify_one(); // Notify the processing thread that a new request is available
}



/**
 * @brief Converts a time string in format "HH:MM:SS.mmm" to milliseconds.
 * @param timeStr The time string.
 * @return The time in milliseconds since 00:00:00.000.
 */
int convertTimeToMs(const std::string& timeStr) {
    int hours, minutes, seconds, millis;
    char colon1, colon2, dot;
    std::istringstream ss(timeStr);
    ss >> hours >> colon1 >> minutes >> colon2 >> seconds >> dot >> millis;

    return ((hours * 3600) + (minutes * 60) + seconds) * 1000 + millis;
}

/**
 * @brief Continuously processes floor requests and communicates with the Scheduler.
 */
void Floor::processRequests() {
    // Mark the start of simulation time
    auto simulationStart = std::chrono::steady_clock::now();

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

            if (request.floor < 1 || request.floor > 10) {
                std::cerr << "[Floor] ERROR: Invalid floor number parsed: " << request.floor << std::endl;
                continue;
            }

            // Calculate how much time has elapsed since simulation start
            auto now = std::chrono::steady_clock::now();
            int elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - simulationStart).count();

            // If simulated request time hasn't passed, wait
            if (elapsed_ms < request.timestamp_ms) {
                int wait_time = request.timestamp_ms - elapsed_ms;
                std::cout << "[Floor] Waiting " << wait_time << " ms before sending request for Floor "
                          << request.floor << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
            }

            std::cout << "[Floor] Processing request for Floor " << request.floor << std::endl;

            // Finally, send to Scheduler
            sendToScheduler(request);
        }
    }
}

/**
 * @brief Stops the Floor subsystem.
 */
void Floor::stop() {
    std::unique_lock<std::mutex> lock(mtx);
    stopSubsystem = true;
    cv.notify_all();
}

/**
 * @brief Reads requests from a file and adds them to the queue.
 * @param filename The name of the input file containing requests.
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

    while (std::getline(inputFile, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string timeStr, direction;
        int floorNum = -1, carButton = -1, fault = 0;

        if (!(iss >> timeStr >> floorNum >> direction >> carButton >> fault)) {
            std::cerr << "[Floor] ERROR: Invalid format in line: '" << line << "'" << std::endl;
            continue;
        }

        if (floorNum < 1 || floorNum > 10) {
            std::cerr << "[Floor] ERROR: Invalid floor number parsed: " << floorNum << std::endl;
            continue;
        }

        bool goingUp = (direction == "Up");

        int timestamp = convertTimeToMs(timeStr);

        // Capture the first timestamp as base
        if (baseTimestamp == -1) {
            baseTimestamp = timestamp;
        }

        // Normalize timestamps relative to the first
        timestamp -= baseTimestamp;

        std::cout << "[Floor] Parsed Request: Time: " << timeStr
                  << " | Floor: " << floorNum
                  << " | Direction: " << (goingUp ? "Up" : "Down")
                  << " | Car Button: " << carButton
                  << " | Fault: " << fault
                  << " | Relative Timestamp: " << timestamp << " ms" << std::endl;

        Request request;
        request.floor = floorNum;
        request.destinationFloor = carButton; // Assuming carButton is the destination
        request.goingUp = goingUp;
        request.faultFlag = fault; // Set the fault flag
        request.timestamp_ms = timestamp; // Set the timestamp
        request.type = FLOOR_REQUEST; // <-- THIS IS MISSING
        request.source = FROM_FLOOR;  // <-- THIS IS MISSING


        // Debugging output
        std::cout << "[Floor] Request created: Floor: " << request.floor
                  << ", Destination: " << request.destinationFloor
                  << ", Going Up: " << (request.goingUp ? "Yes" : "No")
                  << ", Fault Flag: " << request.faultFlag << std::endl;

        addRequestToQueue(request);
    }

    std::cout << "[Floor] Finished reading file: " << filename << std::endl;
}


/**
 * @brief Sends a floor request to the Scheduler via UDP.
 */
void Floor::sendToScheduler(const Request& request) {
    std::cout << "[Floor] Sending request for Floor " << request.floor
              << " (Direction: " << (request.goingUp ? "Up" : "Down") << ") to Scheduler." << std::endl;

    DatagramSocket socket;
    std::vector<uint8_t> data(sizeof(Request));
    memcpy(data.data(), &request, sizeof(Request));
    DatagramPacket packet(data, sizeof(Request), InetAddress::getLocalHost(), SCHEDULER_PORT);
    socket.send(packet);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

/**
 * @brief Receives a notification from the Scheduler when an elevator arrives.
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



size_t Floor::getQueueSize() {
    std::unique_lock<std::mutex> lock(mtx);
    return floorQueue.size();
}

#ifndef TEST_MODE
int main() {
    Floor floor;
    std::thread listenerThread(&Floor::receiveFromScheduler, &floor);
    listenerThread.detach();
    floor.start("input.txt");
    return 0;
}
#endif
