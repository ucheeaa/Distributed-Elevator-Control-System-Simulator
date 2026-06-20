#include <gtest/gtest.h>
#include <fstream>
#include <thread>
#include <cstdio>
#include "Elevator.h"
#include "Floor.h"
#include "Scheduler.h"

TEST(Assignment1Iteration1Test, EndToEndRequestFlowFromFile) {
    // Create Elevator, Floor, and Scheduler
    Elevator* elevator = new Elevator(1);
    std::vector<Elevator*> elevators = { elevator };
    
    // Initialize Floor with the Scheduler in its constructor
    Scheduler* scheduler = new Scheduler(elevators, nullptr);  // Pass nullptr temporarily for floor
    Floor* floor = new Floor(scheduler);  // Now Floor knows about the Scheduler
    
    // Set the scheduler in the floor subsystem after initializing
    scheduler->setFloorSubsystem(floor);  // Set Floor in the Scheduler, allowing communication
    
    // Create test input file
    const std::string filename = "test_input_iteration1.txt";
    std::ofstream testFile(filename);
    testFile << "12:00:01 3 Up 5\n";  // Simulates a user on floor 3 wanting to go up to 5
    testFile.close();

    // Launch subsystems in threads
    std::thread schedulerThread(&Scheduler::processRequests, scheduler);
    std::thread elevatorThread(&Elevator::run, elevator, scheduler);
    std::thread floorThread(&Floor::start, floor, filename);

    // Allow time for system to process
    std::this_thread::sleep_for(std::chrono::seconds(6));

    // Stop all subsystems
    floor->stop();
    scheduler->stop();
    elevator->stop();

    // Join threads
    floorThread.join();
    schedulerThread.join();
    elevatorThread.join();

    // Check result
    EXPECT_EQ(elevator->getCurrentFloor(), 3); // Elevator should have gone to floor 3

    // Cleanup
    delete elevator;
    delete scheduler;
    delete floor;
    std::remove(filename.c_str());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
