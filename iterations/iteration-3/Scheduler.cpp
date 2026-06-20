/**
 * SYSC3303 - Project Iteration 3
 * Author: Uchenna Obikwelu
 * Date: March 8th, 2025
 */

 #include <iostream>
 #include <vector>
 #include <thread>
 #include <climits>     // Added for INT_MAX
 #include "Scheduler.h"
 #include "Elevator.h"
 #include "Floor.h"
 
 #define FLOOR_REQUEST_PORT 50000      // Scheduler listens for requests from Floor
 #define FLOOR_NOTIFICATION_PORT 50002 // Floor listens for elevator arrival notifications
 #define ELEVATOR_BASE_PORT 6000       // Base port for Elevators (each elevator: 6000 + id)
 #define ELEVATOR_STATUS_PORT 50004    // Port for elevator status updates
 
 /**
  * @brief Constructor for the Scheduler.
  */
 Scheduler::Scheduler()
     : state(SchedulerState::IDLE), socket(FLOOR_REQUEST_PORT) {}
 
 /**
  * @brief Adds a new request to the scheduler's queue.
  */
 void Scheduler::addRequest(int floor, int destinationFloor,
                            RequestType type, RequestSource source,
                            bool goingUp, int elevatorId) {
     std::unique_lock<std::mutex> lock(mtx);
     Request request(floor, destinationFloor, type, source, goingUp, elevatorId);
 
     if (type == FLOOR_REQUEST) {
         if (goingUp) {
             upQueue.push(request);
         } else {
             downQueue.push(request);
         }
         cv.notify_one();  // notify processRequests() immediately
     }
 
     state = SchedulerState::PROCESSING;
     std::cout << "[Scheduler] Added FLOOR REQUEST at Floor " << floor
               << " (Direction: " << (goingUp ? "Up" : "Down") << ") to queue" 
               << std::endl;
 }
 
 /**
  * @brief Listens for incoming requests from Floor via UDP.
  */
 void Scheduler::listenForRequests() {
     while (true) {
         std::vector<uint8_t> in(sizeof(Request));
         DatagramPacket receivePacket(in, in.size());
         try {
             socket.receive(receivePacket);
         } catch (const std::runtime_error &e) {
             std::cerr << e.what() << std::endl;
             continue;
         }
 
         Request request;
         memcpy(&request, receivePacket.getData(), sizeof(Request));
         std::cout << "[Scheduler] Received request from Floor " << request.floor
                   << " (Direction: " << (request.goingUp ? "Up" : "Down") << ")" 
                   << std::endl;
         // Add the received floor request to the Scheduler's processing queue
         addRequest(request.floor, request.destinationFloor, FLOOR_REQUEST, FROM_FLOOR, request.goingUp, -1);
     }
 }
 
 /**
  * @brief Receives periodic status updates from elevators.
  */
 void Scheduler::receiveElevatorUpdates() {
     DatagramSocket socket(ELEVATOR_STATUS_PORT);
     while (true) {
         std::vector<uint8_t> data(sizeof(int) * 2);
         DatagramPacket packet(data, data.size());
         try {
             socket.receive(packet);
         } catch (const std::runtime_error &e) {
             std::cerr << "[Scheduler] Error receiving elevator update: " << e.what() << std::endl;
             continue;
         }
 
         // (Optional debugging) print raw bytes
         // std::cout << "[Scheduler] Raw Data Received: ";
         // for (uint8_t byte : data) { std::cout << std::hex << (int)byte << " "; }
         // std::cout << std::endl;
 
         // Parse the received data into elevator ID and floor
         int elevatorId, currentFloor;
         memcpy(&elevatorId, data.data(), sizeof(int));
         memcpy(&currentFloor, data.data() + sizeof(int), sizeof(int));
         // Update the stored floor for this elevator
         elevatorFloors[elevatorId] = currentFloor;
         // Debug log
         std::cout << "[Scheduler] Update: Elevator " << elevatorId 
                   << " is now at Floor " << currentFloor << std::endl;
     }
 }
 
 /**
  * @brief Assigns an elevator to a given floor request.
  * @param request The floor request to assign.
  *
  * Chooses the best elevator based on current position and load.
  */
 void Scheduler::assignElevator(const Request& request) {
     std::unique_lock<std::mutex> lock(mtx);
 
     int bestElevatorId = -1;
     int minDistance = INT_MAX;
     int minLoad = INT_MAX;
 
     // Evaluate each elevator's ability to take the request
     for (const auto& entry : elevatorFloors) {
         int elevatorId = entry.first;
         int elevatorFloor = entry.second;
         int distance = abs(elevatorFloor - request.floor);
         // Determine current load (number of pending requests) for this elevator
         int load = elevatorRequestQueue[elevatorId].size();
         // Check if elevator is already moving in the requested direction (if known)
         bool movingInSameDirection = elevatorDirections.count(elevatorId) 
                                      ? (request.goingUp == elevatorDirections[elevatorId])
                                      : true;
         // Apply a small penalty if the elevator is moving in the opposite direction
         if (!movingInSameDirection) {
             distance += 2;
         }
         // Choose elevator with least load, or if equal load, the one closest by distance
         if ((load < minLoad) || (load == minLoad && distance < minDistance)) {
             minLoad = load;
             minDistance = distance;
             bestElevatorId = elevatorId;
         }
     }
 
     if (bestElevatorId != -1) {
         std::cout << "[Scheduler] Assigned Floor " << request.floor
                   << " to Elevator " << bestElevatorId << std::endl;
         // Record the assignment for that elevator
         elevatorRequestQueue[bestElevatorId].push(request);
         elevatorDirections[bestElevatorId] = request.goingUp;
         // Send the request to the chosen elevator via UDP
         std::vector<uint8_t> data(sizeof(Request));
         memcpy(data.data(), &request, sizeof(Request));
         DatagramPacket sendPacket(data, data.size(),
                                   InetAddress::getLocalHost(), ELEVATOR_BASE_PORT + bestElevatorId);
         try {
             socket.send(sendPacket);
             std::cout << "[Scheduler] Sent assignment to Elevator " << bestElevatorId << std::endl;
         } catch (const std::runtime_error &e) {
             std::cerr << "[Scheduler] Error sending assignment: " << e.what() << std::endl;
         }
     } else {
         std::cout << "[Scheduler] No available elevators for Floor " << request.floor << std::endl;
     }
 }
 
 /**
  * @brief Processes incoming requests and assigns them to elevators.
  */
 void Scheduler::processRequests() {
     while (true) {
         std::unique_lock<std::mutex> lock(mtx);
         // Wait until there's a request or a shutdown signal
         cv.wait(lock, [this]() {
             return !upQueue.empty() || !downQueue.empty() || state == SchedulerState::SHUTTING_DOWN;
         });
         // If shutting down and no requests left, exit the loop
         if (state == SchedulerState::SHUTTING_DOWN && upQueue.empty() && downQueue.empty()) {
             std::cout << "[Scheduler] No more requests. Exiting..." << std::endl;
             return;
         }
 
         Request request;
         bool isFloorRequest = false;
         // Prioritize up-requests, then down-requests
         if (!upQueue.empty()) {
             request = upQueue.front();
             upQueue.pop();
             isFloorRequest = true;
         } else if (!downQueue.empty()) {
             request = downQueue.front();
             downQueue.pop();
             isFloorRequest = true;
         }
         lock.unlock();
 
         if (isFloorRequest) {
             std::cout << "[Scheduler] Processing FLOOR REQUEST at Floor " << request.floor
                       << " -> Destination " << request.destinationFloor
                       << " (" << (request.goingUp ? "Up" : "Down") << ")" << std::endl;
             assignElevator(request);
         }
     }
 }
 
 /**
  * @brief Notifies the Floor subsystem when an elevator arrives at a floor.
  */
 void Scheduler::notifyFloor(int floor) {
     std::cout << "[Scheduler] Notifying Floor subsystem: Elevator arrived at Floor " << floor << std::endl;
     // Send a simple integer (floor number) notification via UDP
     std::vector<uint8_t> data(sizeof(int));
     memcpy(data.data(), &floor, sizeof(int));
     DatagramPacket packet(data, sizeof(int), InetAddress::getLocalHost(), FLOOR_NOTIFICATION_PORT);
     try {
         socket.send(packet);
     } catch (const std::runtime_error &e) {
         std::cerr << "[Scheduler] Error sending floor notification: " << e.what() << std::endl;
     }
 }
 
 /**
  * @brief Notifies that an elevator has completed a request at a floor.
  */
 void Scheduler::notifyCompletion(int elevatorId, int floor, int destinationFloor) {
     std::cout << "[Scheduler] Elevator " << elevatorId << " completed request: Moved from Floor "
               << floor << " to Destination Floor " << destinationFloor << std::endl;
     // Inform the floor subsystem of the elevator's arrival at destination
     notifyFloor(destinationFloor);
 }
 
 /**
  * @brief Returns the last known floor for a given elevator.
  */
 int Scheduler::requestElevatorFloor(int elevatorId) {
     if (elevatorFloors.find(elevatorId) != elevatorFloors.end()) {
         return elevatorFloors[elevatorId];
     } else {
         std::cout << "[Scheduler]  WARNING: No floor update for Elevator " << elevatorId << std::endl;
         return -1; // No data yet for that elevator
     }
 }
 
 /**
  * @brief Stops the scheduler (shuts down processing).
  */
 void Scheduler::stop() {
     std::unique_lock<std::mutex> lock(mtx);
     state = SchedulerState::SHUTTING_DOWN;
     cv.notify_all();
 }
 
 /**
  * @brief Main function to start the Scheduler system.
  */
#ifndef TEST_MODE
int main() {
    try {
        Scheduler scheduler;
        std::thread listenerThread(&Scheduler::listenForRequests, &scheduler);
        std::thread processingThread(&Scheduler::processRequests, &scheduler);
        std::thread updateThread(&Scheduler::receiveElevatorUpdates, &scheduler);

        listenerThread.join();
        processingThread.join();
        updateThread.join();
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
#endif
