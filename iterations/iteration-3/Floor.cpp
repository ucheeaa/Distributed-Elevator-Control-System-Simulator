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
  *
  * Locks the mutex, adds the request to the queue, then notifies the processing thread.
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
  * Called when the Scheduler notifies that an elevator has reached a floor.
  */
 void Floor::handleElevatorArrival(int floor) {
     std::cout << "[Floor] Elevator has arrived at Floor " << floor << std::endl;
 }
 
 /**
  * @brief Continuously processes floor requests and communicates with the Scheduler.
  *
  * Runs in a loop, waiting for new floor requests. Stops when `stopSubsystem` is true and the queue is empty.
  */
 void Floor::processRequests() {
     while (true) {
         std::unique_lock<std::mutex> lock(mtx);
         // Wait for either a new request or a stop signal
         cv.wait(lock, [this]() {
             return !floorQueue.empty() || stopSubsystem;
         });
 
         // If stopping and no more requests, exit the loop
         if (stopSubsystem && floorQueue.empty()) {
             std::cout << "[Floor] No more requests. Stopping..." << std::endl;
             return;
         }
 
         // Process the next request if available
         if (!floorQueue.empty()) {
             Request request = floorQueue.front();
             floorQueue.pop();
             lock.unlock();
 
             std::cout << "[Floor] Processing request for Floor " << request.floor << std::endl;
             sendToScheduler(request);
         }
     }
 }
 
 /**
  * @brief Stops the Floor subsystem.
  *
  * Sets the `stopSubsystem` flag to true and notifies all waiting threads so they can exit.
  */
 void Floor::stop() {
     std::unique_lock<std::mutex> lock(mtx);
     stopSubsystem = true;
     cv.notify_all();
 }
 
 /**
  * @brief Reads requests from a file and adds them to the queue.
  * @param filename The name of the input file containing requests.
  *
  * Expects each line in the file to have the format: "Time Floor Direction CarButton"
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
         if (line.empty()) continue; // Skip empty lines
 
         std::istringstream iss(line);
         std::string timeStr, direction;
         int floor = -1, carButton = -1;
 
         // Parse the input line into components
         if (!(iss >> timeStr >> floor >> direction >> carButton)) {
             std::cerr << "[Floor] ERROR: Invalid format in line: '" << line << "'" << std::endl;
             continue;
         }
 
         // Validate the floor number
         if (floor < 1 || floor > 10) {
             std::cerr << "[Floor] ERROR: Invalid floor number parsed: " << floor << std::endl;
             continue;
         }
 
         // Convert direction string to boolean
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
  * @brief Sends a floor request to the Scheduler via UDP.
  * @param request The request to be sent.
  *
  * Logs the action, then sends the request in a UDP packet to the Scheduler.
  */
 void Floor::sendToScheduler(const Request& request) {
     std::cout << "[Floor] Sending request for Floor " << request.floor
               << " (Direction: " << (request.goingUp ? "Up" : "Down") << ") to Scheduler." << std::endl;
 
     // Prepare UDP packet with the request data
     DatagramSocket socket; // create an unbound socket
     std::vector<uint8_t> data(sizeof(Request));
     memcpy(data.data(), &request, sizeof(Request));
     DatagramPacket packet(data, sizeof(Request), InetAddress::getLocalHost(), SCHEDULER_PORT);
     socket.send(packet);
 
     // Short delay for log readability
     std::this_thread::sleep_for(std::chrono::milliseconds(10));
 }
 
 /**
  * @brief Receives a notification from the Scheduler when an elevator arrives.
  * @details Listens on FLOOR_LISTEN_PORT for an integer indicating the arrived floor.
  */
 void Floor::receiveFromScheduler() {
     DatagramSocket socket(FLOOR_LISTEN_PORT);
     while (true) {
         std::vector<uint8_t> in(sizeof(int));
         DatagramPacket receivePacket(in, in.size());
 
         try {
             socket.receive(receivePacket);
         } catch (const std::runtime_error &e) {
             std::cerr << "[Floor] Error receiving from Scheduler: " << e.what() << std::endl;
             continue;
         }
 
         int arrivedFloor;
         memcpy(&arrivedFloor, receivePacket.getData(), sizeof(int));
         std::cout << "[Floor] Elevator has arrived at Floor " << arrivedFloor << std::endl;
     }
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
