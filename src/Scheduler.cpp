/**
 * SYSC3303 - Project Iteration 5
 * Group: B2-G5
 * Date: April 7th, 2025
 */

 #include <iostream>
 #include <iomanip>
 #include <vector>
 #include <thread>
 #include <climits>
 #include "Scheduler.h"
 #include "Elevator.h"
 #include "Floor.h"
 
 #define FLOOR_REQUEST_PORT 50000      // Scheduler listens for requests from Floor
 #define FLOOR_NOTIFICATION_PORT 50002 // Floor listens for elevator arrival notifications
 #define ELEVATOR_BASE_PORT 6100       // Base port for Elevators (each elevator: 6000 + id)
 #define ELEVATOR_STATUS_PORT 50004    // Port for elevator status updates
 #define ELEVATOR_RETRY_PORT 50006
 
 
 std::map<int, ElevatorStatus> elevatorStatuses;
 std::unordered_map<int, bool> isMoving; // true = in motion, false = reached
 
 /**
  * @brief Constructor for the Scheduler.
  * Initializes the scheduler with the default floor request socket.
  */
 Scheduler::Scheduler()
         : state(SchedulerState::IDLE), socket(FLOOR_REQUEST_PORT) {
     socketPtr = &socket;
 }
 
 Scheduler::Scheduler(DatagramSocket* customSocket) : state(IDLE), socketPtr(customSocket) {}
 
 /**
  * @brief Custom constructor for the Scheduler.
  * Allows for injecting a custom socket for UDP communication.
  * @param customSocket A pointer to a custom DatagramSocket.
  */
 //Scheduler::Scheduler(DatagramSocket* customSocket)
         //: state(IDLE), socketPtr(customSocket) {}
 
 /**
  * @brief Adds a new request to the scheduler's queue.
  * Places the request in either the "up" or "down" queue based on the requested direction.
  * Notifies the `processRequests` function immediately.
  * @param floor The floor where the request was made.
  * @param destinationFloor The destination floor for the request.
  * @param type Type of the request (floor or elevator).
  * @param source The source of the request (Floor or Elevator).
  * @param goingUp Whether the request is for moving up (true) or down (false).
  * @param elevatorId The ID of the elevator if the request comes from an elevator.
  * @param faultFlag The fault flag to determine if the elevator is faulty.
  */
 void Scheduler::addRequest(int floor, int destinationFloor,
                            RequestType type, RequestSource source,
                            bool goingUp, int elevatorId,  int faultFlag) {
     std::unique_lock<std::mutex> lock(mtx);
     Request request(floor, destinationFloor, type, source, goingUp, elevatorId, 0, faultFlag);
 
     if (type == FLOOR_REQUEST) {
         if (goingUp) {
             upQueue.push(request);     // add to upQueue
         } else {
             downQueue.push(request);     // add to downQueue
         }
         //cv.notify_one();  // notify processRequests() immediately
     } else if (type == ELEVATOR_REQUEST) {
         if (goingUp) {
             upQueue.push(request);
         } else {
             downQueue.push(request);
         }
     }
 
     cv.notify_one();
 
     state = SchedulerState::PROCESSING;        // set state to PROCESSING
     std::cout << "[Scheduler] Added FLOOR REQUEST at Floor " << floor
               << " (Direction: " << (goingUp ? "Up" : "Down") << ") to queue"
               << std::endl;
 }
 
 /**
  * @brief Listens for incoming requests from the Floor system.
  * Receives UDP packets with floor requests and adds them to the scheduler queue.
  */
 void Scheduler::listenForRequests() {
     while (true) {
         std::vector<uint8_t> in(sizeof(Request));
         DatagramPacket receivePacket(in, in.size());
         try {
             socket.receive(receivePacket); // Wait for floor request
         } catch (const std::runtime_error &e) {
             std::cerr << e.what() << std::endl;
             continue;
         }
 
         Request request;
         memcpy(&request, receivePacket.getData(), sizeof(Request));
 
         
         std::cout << "[Scheduler] Received request from Floor " << request.floor
                   << " (Direction: " << (request.goingUp ? "Up" : "Down") << ")"
                   << std::endl;
         // Add the request to the scheduler's queue
         addRequest(request.floor, request.destinationFloor, FLOOR_REQUEST, FROM_FLOOR, request.goingUp, -1, request.faultFlag);
     }
 }
 
 
 /**
  * @brief Receives periodic elevator status updates and detects faults.
  * Updates the scheduler with elevator location and status information.
  * @details Handles elevator heartbeat updates, completed trips, and fault reports.
  */
 void Scheduler::receiveElevatorUpdates() {
     DatagramSocket socket(ELEVATOR_STATUS_PORT);
 
     while (true) {
         std::vector<uint8_t> data(sizeof(int) * 5);
         DatagramPacket packet(data, data.size());
 
         try {
             socket.receive(packet);
         } catch (const std::runtime_error &e) {
             std::cerr << "[Scheduler] Error receiving elevator update: " << e.what() << std::endl;
             continue;
         }
 
         int elevatorId, origin, pickup, dest, statusType;
         memcpy(&elevatorId, data.data(), sizeof(int));
         memcpy(&origin, data.data() + sizeof(int), sizeof(int));
         memcpy(&pickup, data.data() + 2 * sizeof(int), sizeof(int));
         memcpy(&dest, data.data() + 3 * sizeof(int), sizeof(int));
         memcpy(&statusType, data.data() + 4 * sizeof(int), sizeof(int));
 
         switch (static_cast<StatusType>(statusType)) {
             case STATUS_UPDATE: {
             int prevFloor = elevatorFloors.count(elevatorId) ? elevatorFloors[elevatorId] : -1;
             elevatorFloors[elevatorId] = origin;
 
             if (prevFloor != -1 && prevFloor != origin) {
                 elevatorStatuses[elevatorId].state = ElevatorState::MOVING_UP; // or DOWN, doesn't matter for now
                 isMoving[elevatorId] = true;
             } else if (prevFloor == origin && isMoving[elevatorId]) {
                 elevatorStatuses[elevatorId].state = ElevatorState::IDLE;
                 isMoving[elevatorId] = false;  // mark as REACHED
                 std::cout << "[Scheduler] Elevator " << elevatorId << " has REACHED destination: Floor " << origin << std::endl;
             }
 
             break;
     }
 
             case STATUS_COMPLETE: {
                 std::cout << "[Scheduler] Elevator " << elevatorId
                           << " completed trip: Came from Floor " << origin
                           << ", Picked up at Floor " << pickup
                           << ", Destination Floor " << dest << std::endl;
 
                 notifyFloor(elevatorId, origin, pickup, dest);
 
                 #ifdef TEST_MODE
                     markRequestCompleted();
                 #endif
 
                 break;
             }
 
             case STATUS_HARD_FAULT: {
                 std::cerr << "[Scheduler] FAULT reported by Elevator " << elevatorId
                           << " at Floor " << origin << ". Marking as FAULTY." << std::endl;
                 elevatorFaults[elevatorId] = true;
                 elevatorStatuses[elevatorId].state = ElevatorState::OUT_OF_SERVICE;
                 broadcastElevatorStatus();
 
                 #ifdef TEST_MODE
                     markElevatorFaulty(elevatorId);
                 #endif
 
                 break;
             }
             case STATUS_TRANSIENT_FAULT: {
                 std::cout << "[Scheduler] TRANSIENT FAULT reported by Elevator " << elevatorId
                           << " at Floor " << origin << ". Recovered after retries." << std::endl;
 
                 #ifdef TEST_MODE
                     setTransientFaultRecovered();
                 #endif
 
                 break;
             }
 
 
             default:
                 std::cerr << "[Scheduler] Unknown status type: " << statusType << std::endl;
                 break;
         }
     }
 }
 
 void Scheduler::listenForElevatorRequests() {
     DatagramSocket socket(ELEVATOR_RETRY_PORT); // define ELEVATOR_RETRY_PORT = 50006
 
     #ifdef TEST_MODE
         incrementRetryCount();
     #endif
 
     while (true) {
         std::vector<uint8_t> in(sizeof(Request));
         DatagramPacket receivePacket(in, in.size());
         try {
             socket.receive(receivePacket);
         } catch (const std::runtime_error &e) {
             std::cerr << "[Scheduler] Error receiving elevator retry: " << e.what() << std::endl;
             continue;
         }
 
         Request request;
         memcpy(&request, receivePacket.getData(), sizeof(Request));
         addRequest(request.floor, request.destinationFloor, request.type, request.source, request.goingUp, request.elevatorId, request.faultFlag);
         std::cout << "[Scheduler] Added "
                   << (request.type == FLOOR_REQUEST ? "FLOOR" : "RETRY")
                   << " REQUEST at Floor " << request.floor
                   << " (Direction: " << (request.goingUp ? "Up" : "Down") << ") to queue"
                   << std::endl;
 
     }
 }
 
 /**
  * @brief Assigns the best available elevator to handle a floor request.
  * Skips faulty elevators and selects the one with the closest proximity and least load.
  * @param request The floor request that needs to be assigned to an elevator.
  */
 void Scheduler::assignElevator(const Request& request) {
     int bestElevatorId = -1;
     int minDistance = INT_MAX;
     int minLoad = INT_MAX;
 
     for (const auto& entry : elevatorFloors) {
         int elevatorId = entry.first;
         int elevatorFloor = entry.second;
 
         // Skip faulty elevators
         if (elevatorFaults[elevatorId]  || elevatorStates[elevatorId] == ElevatorState::OUT_OF_SERVICE) {
             std::cout << "[Scheduler] Skipping elevator " << elevatorId << " due to FAULT or OUT_OF_SERVICE\n";
             continue;
         }
 
         int distance = abs(elevatorFloor - request.floor);
         int load = elevatorRequestQueue[elevatorId].size();
         bool movingInSameDirection = elevatorDirections.count(elevatorId) ?
                                      (request.goingUp == elevatorDirections[elevatorId]) : true;
         if (!movingInSameDirection) distance += 2; // Small penalty for opposite direction
 
         // Select the elevator with the least load and shortest distance
         if ((load < minLoad) || (load == minLoad && distance < minDistance)) {
             minLoad = load;
             minDistance = distance;
             bestElevatorId = elevatorId;
         }
     }
 
     if (bestElevatorId != -1) {
         //previousFloor[bestElevatorId] = elevatorFloors[bestElevatorId];
 
         if (elevatorFloors.find(bestElevatorId) != elevatorFloors.end()) {
             previousFloor[bestElevatorId] = elevatorFloors[bestElevatorId];
         } else {
             std::cout << "[Scheduler] WARNING: No floor data for Elevator " << bestElevatorId
                       << ". Defaulting origin to 0.\n";
             previousFloor[bestElevatorId] = 0;
         }
 
         std::cout << "[Scheduler] Assigned Floor " << request.floor << " to Elevator " << bestElevatorId << std::endl;
         elevatorRequestQueue[bestElevatorId].push(request);
         elevatorDirections[bestElevatorId] = request.goingUp;
 
         pickupFloor[bestElevatorId] = request.floor;
 
 
         // Send request to the assigned elevator
         std::vector<uint8_t> data(sizeof(Request));
         memcpy(data.data(), &request, sizeof(Request));
         DatagramPacket sendPacket(data, data.size(),
                                   InetAddress::getLocalHost(), ELEVATOR_BASE_PORT + bestElevatorId);
         try {
             socket.send(sendPacket); // Send the request to the elevator
             std::cout << "[Scheduler] Sent assignment to Elevator " << bestElevatorId << std::endl;
         } catch (const std::runtime_error &e) {
             std::cerr << "[Scheduler] Error sending assignment: " << e.what() << std::endl;
         }
     } else {
         std::cerr << "[Scheduler] No available elevators for Floor " << request.floor << std::endl;
     }
 }
 
 /**
  * @brief Periodically checks for stuck elevators.
  * If no update is received in 15+ seconds, marks the elevator as faulty.
  */
 void Scheduler::checkForStuckElevators() {
     while (true) {
         std::this_thread::sleep_for(std::chrono::seconds(5));
 
         auto now = std::chrono::steady_clock::now();
         for (auto& entry : elevatorLastUpdate) {
             int elevatorId = entry.first;
             auto lastUpdateTime = entry.second;
 
             if (elevatorFaults[elevatorId]) continue;  // already marked faulty
 
             // NEW: Check if the elevator is idle — skip check if so
             if (elevatorStatuses[elevatorId].state == ElevatorState::IDLE) {
                 // Optional: you can log that it's idle if needed
                 continue;
             }
 
             // OLD: timeout-based hard fault detection
             if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdateTime).count() > 15) {
                 std::cerr << "[Scheduler] WATCHDOG: Elevator " << elevatorId << " has not reported in 15s and is NOT idle! Marking as stuck." << std::endl;
                 elevatorFaults[elevatorId] = true;
 
                 // Optionally notify the GUI or print to terminal
             }
         }
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
                       << " (" << (request.goingUp ? "Up" : "Down") << ")" << ", Fault Flag: " << request.faultFlag << std::endl;
 
             // Always assign elevator, even if fault flag == 1
             assignElevator(request);
 
         }
     }
 }
 
 /**
  * @brief Notifies the Floor subsystem when an elevator arrives.
  */
 void Scheduler::notifyFloor(int elevatorId, int originFloor, int pickupFloor, int destinationFloor) {
 
     std::vector<uint8_t> data(sizeof(int) * 4);
     memcpy(data.data(), &elevatorId, sizeof(int));
     memcpy(data.data() + sizeof(int), &originFloor, sizeof(int));
     memcpy(data.data() + 2 * sizeof(int), &pickupFloor, sizeof(int));
     memcpy(data.data() + 3 * sizeof(int), &destinationFloor, sizeof(int));
 
     DatagramPacket packet(data, data.size(), InetAddress::getLocalHost(), FLOOR_NOTIFICATION_PORT);
 
     try {
         socketPtr->send(packet);
     } catch (const std::runtime_error &e) {
         std::cerr << "[Scheduler] Error sending floor notification: " << e.what() << std::endl;
     }
 }
 
 void Scheduler::broadcastElevatorStatus() {
     for (const auto& entry : elevatorFloors) {
 
         int elevatorId = entry.first;
         int floor = entry.second;
         int isFaulty = elevatorFaults[elevatorId] ? 1 : 0;
 
         std::vector<uint8_t> data(sizeof(int) * 3);
         memcpy(data.data(), &elevatorId, sizeof(int));
         memcpy(data.data() + sizeof(int), &floor, sizeof(int));
         memcpy(data.data() + 2 * sizeof(int), &isFaulty, sizeof(int));
 
         DatagramPacket packet(data, data.size(), InetAddress::getLocalHost(), 50010);
         try {
             socket.send(packet);
         } catch (const std::runtime_error& e) {
             std::cerr << "[Scheduler] Error sending update to DisplayConsole: " << e.what() << std::endl;
         }
     }
 }
 
 //For Integration Testing
 void Scheduler::run() {
     std::thread t1(&Scheduler::processRequests, this);
     std::thread t2(&Scheduler::receiveElevatorUpdates, this);
     std::thread t3(&Scheduler::listenForRequests, this);
     std::thread t4(&Scheduler::listenForElevatorRequests, this);
 
     t1.join();
     t2.join();
     t3.join();
     t4.join();
 }
 
 /**
  * @brief Stops the scheduler (shuts down processing).
  */
 void Scheduler::stop() {
     std::unique_lock<std::mutex> lock(mtx);
     state = SchedulerState::SHUTTING_DOWN;
     cv.notify_all();
 }
 
 void Scheduler::displayStatusLoop() {
     while (true) {
         std::this_thread::sleep_for(std::chrono::seconds(2));
         std::cout << "\n───────────────────────────────\n";
         std::cout << " Elevator | Floor | State\n";
         std::cout << "───────────────────────────────\n";
     std::string stateStr;
         for (const auto& pair : elevatorStatuses) {
                int id = pair.first;
                const ElevatorStatus& status = pair.second;
            
         switch (status.state) {
             case ElevatorState::MOVING_UP:
             case ElevatorState::MOVING_DOWN:
                 stateStr = "MOVING"; break;
             case ElevatorState::IDLE:
             case ElevatorState::DOOR_OPEN:
                 stateStr = (isMoving[id] ? "MOVING" : "REACHED");
             break;
             case ElevatorState::FAULTY:
                 stateStr = "FAULTY"; break;
             case ElevatorState::OUT_OF_SERVICE:
                 stateStr = "OUT OF SERVICE"; break;
             default:
             stateStr = "UNKNOWN"; break;
     }
 
 
         
 
             int floor = elevatorFloors.count(id) ? elevatorFloors.at(id) : -1;
 
             std::cout << "    " << std::setw(3) << id
                       << "    |  " << std::setw(4) << (floor == -1 ? "-" : std::to_string(floor))
                       << " | " << stateStr << "\n";
         }
 
         std::cout << "───────────────────────────────\n";
         }
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
         std::thread faultCheckThread(&Scheduler::checkForStuckElevators, &scheduler);
         std::thread retryListenerThread(&Scheduler::listenForElevatorRequests, &scheduler);
         std::thread displayThread(&Scheduler::displayStatusLoop, &scheduler);
 
         listenerThread.join();
         processingThread.join();
         updateThread.join();
         faultCheckThread.join();
         retryListenerThread.join();
         displayThread.join();
         
     } catch (const std::runtime_error &e) {
         std::cerr << e.what() << std::endl;
     }
 }
 #endif