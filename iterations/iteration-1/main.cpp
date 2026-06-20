#include "Scheduler.h"
#include "Elevator.h"
#include "Floor.h"
#include <iostream>
#include <thread>
#include <vector>

// Function to test reading input file and passing data back and forth
void testFileReading(Scheduler& scheduler, Floor& floorSubsystem, const std::string& inputFilename) {
    std::cout << "Starting Floor Subsystem to read requests from file: " << inputFilename << "\n";
    std::thread floorThread(&Floor::start, &floorSubsystem, inputFilename);

    // Join the floor thread after it completes
    floorThread.join();
}

// Function to test elevator operations and scheduling
void testElevatorRequests(Scheduler& scheduler, std::vector<Elevator*>& elevators) {
    std::cout << "\nAdding Test Requests from Inside Elevator\n";

    // Test requests from inside the elevator
    scheduler.addRequest(4, ELEVATOR_REQUEST, FROM_ELEVATOR, true, 0);  // Elevator 0 user presses floor 4
    scheduler.addRequest(1, ELEVATOR_REQUEST, FROM_ELEVATOR, false, 0); // Elevator 0 user presses floor 1

    // Start Scheduler processing requests
    std::thread schedulerThread(&Scheduler::processRequests, &scheduler);

    // Start Elevator threads
    std::vector<std::thread> elevatorThreads;
    for (Elevator* elevator : elevators) {
        elevatorThreads.emplace_back(&Elevator::run, elevator, &scheduler);
    }

    // Allow some time for requests to process
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Wait for threads to complete
    schedulerThread.join();
    for (std::thread& t : elevatorThreads) {
        t.join();
    }
}

int main() {
    // Create one elevator
    Elevator* elevator = new Elevator(0); // Initialize elevator with ID 0

    // Wrap the single elevator pointer in a vector
    std::vector<Elevator*> elevators;
    elevators.push_back(elevator);

    // Create Scheduler
    Scheduler scheduler(elevators, nullptr); // Scheduler manages elevators and communicates with Floor

    // Create Floor Subsystem
    Floor floorSubsystem(&scheduler);
    scheduler.setFloorSubsystem(&floorSubsystem); // Establish two-way communication

    // Test file reading and communication
    std::string inputFilename = "input.txt"; // Ensure input.txt is in the same directory
    testFileReading(scheduler, floorSubsystem, inputFilename);

    // Test elevator operations and request scheduling
    testElevatorRequests(scheduler, elevators);

    // Explicitly stop all subsystems before exiting
    std::cout << "Stopping Floor Subsystem...\n";
    floorSubsystem.stop();  // Stop the floor system

    std::cout << "Stopping Scheduler...\n";
    scheduler.stop();  // Stop the scheduler

    std::cout << "Stopping Elevators...\n";
    for (Elevator* elevator : elevators) {
        elevator->stop();  // Stop each elevator
    }

    // Clean up
    for (Elevator* elevator : elevators) {
        delete elevator;
    }

    std::cout << "Simulation complete!" << std::endl;
    return 0;
}