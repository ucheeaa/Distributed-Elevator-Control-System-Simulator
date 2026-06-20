#include <gtest/gtest.h>
#include <fstream>
#include <thread>
#include <cstdio>
#include "Elevator.h"
#include "Floor.h"
#include "Scheduler.h"

TEST(Iteration2Test, ElevatorStateMachineAndSchedulerStateTransition) {
    // Create Elevator, Floor, and Scheduler
    Elevator* elevator = new Elevator(1);
    std::vector<Elevator*> elevators = { elevator };
    Scheduler* scheduler = new Scheduler(elevators, nullptr);
    Floor* floor = new Floor(scheduler);
    scheduler->setFloorSubsystem(floor);

    // Create test input file with a request
    const std::string filename = "test_input_iteration2.txt";
    std::ofstream testFile(filename);
    testFile << "12:00:01 3 Up 5\n";  // Request to go from floor 3 to 5 (Up)
    testFile.close();

    // Start subsystems in separate threads
    std::thread schedulerThread(&Scheduler::processRequests, scheduler);
    std::thread elevatorThread(&Elevator::run, elevator, scheduler);
    std::thread floorThread(&Floor::start, floor, filename);

    // Allow time for the elevator to receive the request and begin processing
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Check that the elevator's state has transitioned to MOVING_UP
    EXPECT_TRUE(elevator->isMovingUp());  

    // Allow elevator to complete the request (including time for door opening/closing)
    std::this_thread::sleep_for(std::chrono::seconds(7));

    // Check that Elevator is now at the correct floor (should be at floor 5 after moving up)
    EXPECT_EQ(elevator->getCurrentFloor(), 5);  

    // Check that the elevator's state has transitioned to IDLE
    EXPECT_TRUE(elevator->isIdle()); 


    // Clean shutdown and join threads
    floor->stop();
    scheduler->stop();
    elevator->stop();

    floorThread.join();
    schedulerThread.join();
    elevatorThread.join();

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
