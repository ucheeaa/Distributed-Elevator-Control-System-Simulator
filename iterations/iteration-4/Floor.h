/**
 * SYSC3303 - Project Iteration 1 
 * Author: Deborah Ajayi
 * Date: February 1st, 2025
 */

 #ifndef FLOOR_H  
 #define FLOOR_H  
 
 #include <iostream>
 #include <fstream>
 #include <thread>
 #include <queue>
 #include <string>
 #include <mutex>
 #include <condition_variable>
 #include "Scheduler.h"
 #include "Datagram.h" 
 
 class Floor {
 private:
     std::queue<Request> floorQueue;           // Queue for floor requests
     std::mutex mtx;                           // Mutex for thread synchronization
     std::condition_variable cv;               // Condition variable for synchronization
     Scheduler* scheduler;                     // Pointer to Scheduler instance
     bool stopSubsystem;                       // Flag to stop the subsystem
     std::ifstream inputFile;                  // Input file stream to read requests
     DatagramSocket socket;
 
 public:
     /**
      * @brief Constructor to initialize the Floor subsystem.
      */
     Floor();
 
     /**
      * @brief Starts the Floor, processing requests from the file.
      * @param filename The name of the file containing requests to process
      */
     void start(const std::string& filename);
 
     /**
      * @brief Stops the Floor subsystem and terminates the processing thread.
      */
     void stop();
 
     /**
      * @brief Adds a request to the floor subsystem's queue.
      * @param request The request to be added
      */
     void addRequestToQueue(const Request& request);
 
     /**
      * @brief Processes floor requests and communicates with the Scheduler.
      */
     void processRequests();
 
     /**
      * @brief Reads requests from a file and adds them to the queue.
      * @param filename Name of the file containing requests
      */
     void readRequestsFromFile(const std::string& filename);
 
     /**
      * @brief Receives a request status update from the Scheduler.
      */
     void receiveFromScheduler();
 
     /**
      * @brief Sends a request to the Scheduler.
      * @param request The request to be sent
      */
     void sendToScheduler(const Request& request);
 
     /**
      * @brief Simulates a passenger arriving at a floor and requesting an elevator.
      * @param floor The floor where the passenger arrives.
      * @param goingUp True if the passenger wants to go up, false if down.
      */
     void simulatePassengerArrival(int floor, bool goingUp);
 
     /**
      * @brief Simulates waiting for requests from passengers.
      */
     void simulateWaitingForRequests();
 
     /**
      * @brief Handles the elevator arrival notification.
      * @param floor The floor where the elevator has arrived.
      */
     void handleElevatorArrival(int floor);

     size_t getQueueSize();
 };
 
 #endif // FLOOR_H 