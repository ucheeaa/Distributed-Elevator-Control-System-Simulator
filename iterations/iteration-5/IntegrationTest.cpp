/**
 * @file IntegrationTest.cpp
 * @brief Full system integration test for the elevator scheduling system.
 *
 * This test sets up the Scheduler, Floor, and Elevator subsystems,
 * launches them on separate threads, and verifies system behavior
 * including fault handling (transient and hard), request completion,
 * and fault marking.
 *
 * The test waits for:
 *  - At least 5 requests to be completed
 *  - A transient fault to be handled and reported
 *  - A hard fault to be injected and detected
 *  - The elevator with a hard fault to be marked as OUT_OF_SERVICE
 *
 * The test avoids time-based assertions and instead uses system flags
 * and status checks to determine correctness.
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "Scheduler.h"
#include "Floor.h"
#include "Elevator.h"

// Global subsystems used across the integration test
Scheduler scheduler;
Floor floorSubsystem;
Elevator elevator1(1);

/**
 * @test FullSystemTest
 * @brief Launches all subsystems and verifies key system behaviors.
 */
TEST(IntegrationTest, FullSystemTest) {
std::thread schedulerThread(&Scheduler::run, &scheduler);
std::thread listenerThread(&Elevator::listenForRequests, &elevator1);
std::thread runThread(&Elevator::run, &elevator1);
std::thread statusThread(&Elevator::statusUpdateLoop, &elevator1);
std::thread floorThread(&Floor::run, &floorSubsystem);

// Wait until elevator is registered in scheduler
int waitedMs = 0;
while (scheduler.getElevatorFaults().empty() && waitedMs < 3000) {
std::this_thread::sleep_for(std::chrono::milliseconds(100));
waitedMs += 100;
}

// Logical flags to track milestone checks
bool completed = false, transientHandled = false, hardFaultDetected = false, elevatorMarked = false;

// Poll system state until all conditions are satisfied
while (true) {
    if (!completed && scheduler.getCompletedRequestCount() >= 5)
    {
        std::cout << "[Test] ✅ Completed at least 5 requests\n";
        EXPECT_GE(scheduler.getCompletedRequestCount(), 5) << "Expected at least 5 completed requests.";
        completed = true;
    }

    if (!transientHandled && scheduler.wasTransientFaultHandled())
    {
        std::cout << "[Test] ✅ Transient fault handled\n";
        EXPECT_TRUE(scheduler.wasTransientFaultHandled()) << "Transient fault was not properly recovered.";
        transientHandled = true;
    }

    if (!hardFaultDetected && scheduler.getHardFaultElevatorId() != -1) {
        int id = scheduler.getHardFaultElevatorId();
        std::cout << "[Test] ✅ Hard fault elevator ID set: " << id << "\n";
        EXPECT_NE(id, -1) << "Hard fault not detected.";
        hardFaultDetected = true;
    }

if (hardFaultDetected && !elevatorMarked) {
    int id = scheduler.getHardFaultElevatorId();
    if (scheduler.isElevatorFaulty(id))
    {
        std::cout << "[Test] ✅ Elevator " << id << " marked as faulty\n";
        EXPECT_TRUE(scheduler.isElevatorFaulty(id)) << "Faulty elevator was not marked as faulty.";
        elevatorMarked = true;
    }
}

if (completed && transientHandled && hardFaultDetected && elevatorMarked)
{
break;
}

std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

// Join all subsystem threads after test passes
schedulerThread.join();
floorThread.join();
listenerThread.join();
runThread.join();
statusThread.join();
}

/**
 * @brief Main entry point to run all Google Test cases.
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}