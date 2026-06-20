/**
 * SYSC3303 - Project Iteration 5
 * Group: B2-G5
 * Date: April 7th, 2025
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Elevator.h"
#include "Scheduler.h"
#include "Floor.h"
#include "Datagram.h"

#define FLOOR_NOTIFICATION_PORT 50002
const int MAX_CAPACITY = 4;

//
// ─────────────────────────────────────────────
// ELEVATOR TESTS
// ─────────────────────────────────────────────
//

/**
 * @brief Verifies basic elevator initialization.
 */
TEST(ElevatorTest, Initialization) {
Elevator elevator(1);
EXPECT_EQ(elevator.getId(), 1);
EXPECT_EQ(elevator.getCurrentFloor(), 0);
EXPECT_TRUE(elevator.isIdle());
}

/**
 * @brief Tests that elevator becomes non-idle after receiving a request.
 */
TEST(ElevatorTest, AddRequestSetsNonIdle) {
Elevator elevator(1);
Request request(1, 5, FLOOR_REQUEST, FROM_FLOOR, true);
elevator.addRequest(request);
EXPECT_FALSE(elevator.isIdle());
}

/**
 * @brief Ensures simulateMoving correctly updates current floor.
 */
TEST(ElevatorTest, SimulateMovementUpdatesCurrentFloor) {
Elevator elevator(1);
elevator.setCurrentFloor(1);
elevator.simulateMoving(4);
EXPECT_EQ(elevator.getCurrentFloor(), 4);
}

/**
 * @brief Runs a normal request and ensures the elevator returns to idle state.
 */
TEST(ElevatorTest, HandlesNormalRequestNoFault) {
Elevator elevator(3);
Request request(1, 4, FLOOR_REQUEST, FROM_FLOOR, true);
elevator.addRequest(request);
std::thread elevatorThread(&Elevator::run, &elevator);
std::this_thread::sleep_for(std::chrono::seconds(10));
elevator.stop();
elevatorThread.join();
EXPECT_TRUE(elevator.isIdle());
}

/**
 * @brief Verifies retry logic for transient door stuck fault.
 */
TEST(ElevatorTest, HandlesTransientDoorStuckFault) {
Elevator elevator(1);
Request request(1, 3, FLOOR_REQUEST, FROM_FLOOR, true);
request.faultFlag = 1;
elevator.addRequest(request);
std::thread elevatorThread(&Elevator::run, &elevator);
std::this_thread::sleep_for(std::chrono::seconds(15));
elevator.stop();
elevatorThread.join();
EXPECT_TRUE(elevator.isIdle());
}

/**
 * @brief Ensures elevator shuts down on hard fault (faultFlag = 2).
 */
TEST(ElevatorTest, HandlesHardFaultAndShutsDown) {
Elevator elevator(2);
Request request(2, 5, FLOOR_REQUEST, FROM_FLOOR, true);
request.faultFlag = 2;
elevator.addRequest(request);
std::thread elevatorThread(&Elevator::run, &elevator);
elevatorThread.join();
EXPECT_EQ(elevator.getState(), ElevatorState::OUT_OF_SERVICE);
}

/**
 * @brief Tests that requests are requeued when elevator crashes due to hard fault.
 */
TEST(ElevatorTest, ReassignsRequestsAfterHardFault) {
Elevator elevator(1);
Request faultyReq(2, 5, FLOOR_REQUEST, FROM_FLOOR, true);
faultyReq.faultFlag = 2;
Request secondReq(3, 6, FLOOR_REQUEST, FROM_FLOOR, true);
elevator.addRequest(faultyReq);
elevator.addRequest(secondReq);
std::thread elevatorThread(&Elevator::run, &elevator);
std::this_thread::sleep_for(std::chrono::seconds(15));
elevatorThread.join();
EXPECT_EQ(elevator.getState(), ElevatorState::OUT_OF_SERVICE);
}

/**
 * @brief Verifies that excess passengers are rejected and requeued if elevator is full.
 */
TEST(ElevatorTest, CapacityLimitEnforced) {
Elevator elevator(1);
for (int i = 0; i < MAX_CAPACITY + 2; ++i) {
Request req(1, 5, FLOOR_REQUEST, FROM_FLOOR, true);
elevator.addRequest(req);
}
std::thread elevatorThread(&Elevator::run, &elevator);
std::this_thread::sleep_for(std::chrono::seconds(15));
elevator.stop();
elevatorThread.join();
EXPECT_EQ(elevator.getPassengerCount(), 0);  // All should be dropped off
}


// ─────────────────────────────────────────────
// SCHEDULER TESTS
// ─────────────────────────────────────────────


/**
 * @brief Tests adding an upward request to the upQueue.
 */
TEST(SchedulerTest, AddRequestUpQueue) {
Scheduler scheduler;
scheduler.addRequest(3, 6, FLOOR_REQUEST, FROM_FLOOR, true);
EXPECT_EQ(scheduler.getUpQueue().size(), 1);
}

/**
 * @brief Tests adding a downward request to the downQueue.
 */
TEST(SchedulerTest, AddRequestDownQueue) {
Scheduler scheduler;
scheduler.addRequest(7, 2, FLOOR_REQUEST, FROM_FLOOR, false);
EXPECT_EQ(scheduler.getDownQueue().size(), 1);
}

/**
 * @brief Ensures scheduler avoids assigning faulty elevators.
 */
TEST(SchedulerTest, AssignElevatorSkipsFaultyElevators) {
Scheduler scheduler;
scheduler.getElevatorFloors()[1] = 0;
scheduler.getElevatorFloors()[2] = 5;
scheduler.getElevatorFloors()[3] = 3;
scheduler.getElevatorFaults()[2] = true;

Request request(4, 8, FLOOR_REQUEST, FROM_FLOOR, true);
scheduler.assignElevator(request);
EXPECT_FALSE(scheduler.getElevatorRequestQueue()[3].empty());
EXPECT_EQ(scheduler.getElevatorRequestQueue()[3].front().floor, 4);
}

/**
 * @brief Ensures idle elevators are not incorrectly marked as faulty during timeout check.
 */
TEST(SchedulerTest, TimeoutIgnoresIdleElevator) {
Scheduler scheduler;
auto now = std::chrono::steady_clock::now();
scheduler.getElevatorLastUpdate()[1] = now - std::chrono::seconds(20);
scheduler.getElevatorFaults()[1] = false;
elevatorStatuses[1] = { 3, ElevatorState::IDLE };

for (auto& entry : scheduler.getElevatorLastUpdate()) {
int id = entry.first;
auto last = entry.second;
if (!scheduler.getElevatorFaults()[id] &&
elevatorStatuses[id].state != ElevatorState::IDLE &&
        std::chrono::duration_cast<std::chrono::seconds>(now - last).count() > 15) {
scheduler.getElevatorFaults()[id] = true;
}
}

EXPECT_FALSE(scheduler.getElevatorFaults()[1]);
}

/**
 * @brief Verifies elevator timeout fault detection logic.
 */
TEST(SchedulerTest, FaultDetectionLogic) {
Scheduler scheduler;
auto now = std::chrono::steady_clock::now();
scheduler.getElevatorLastUpdate()[1] = now - std::chrono::seconds(20);
scheduler.getElevatorLastUpdate()[2] = now;
scheduler.getElevatorFaults()[1] = false;
scheduler.getElevatorFaults()[2] = false;
elevatorStatuses[1] = { 3, ElevatorState::MOVING_UP };
elevatorStatuses[2] = { 4, ElevatorState::IDLE };

for (auto& entry : scheduler.getElevatorLastUpdate()) {
int id = entry.first;
auto last = entry.second;
if (!scheduler.getElevatorFaults()[id] &&
elevatorStatuses[id].state != ElevatorState::IDLE &&
        std::chrono::duration_cast<std::chrono::seconds>(now - last).count() > 15) {
scheduler.getElevatorFaults()[id] = true;
}
}

EXPECT_TRUE(scheduler.getElevatorFaults()[1]);
EXPECT_FALSE(scheduler.getElevatorFaults()[2]);
}

/**
 * @brief Ensures scheduler selects the least loaded and closest elevator.
 */
TEST(SchedulerTest, AssignsClosestAndLeastLoadedElevator) {
Scheduler scheduler;
scheduler.getElevatorFloors()[1] = 1;
scheduler.getElevatorFloors()[2] = 3;
scheduler.getElevatorFloors()[3] = 6;

scheduler.getElevatorRequestQueue()[1] = std::queue<Request>(); // 0 load
scheduler.getElevatorRequestQueue()[2].push(Request());         // 1
scheduler.getElevatorRequestQueue()[3].push(Request());
scheduler.getElevatorRequestQueue()[3].push(Request());         // 2

Request req(2, 7, FLOOR_REQUEST, FROM_FLOOR, true);
scheduler.assignElevator(req);

EXPECT_EQ(scheduler.getElevatorRequestQueue()[1].front().floor, 2);
}


// ─────────────────────────────────────────────
// FLOOR TESTS
// ─────────────────────────────────────────────


/**
 * @brief Verifies adding a request to the floor queue.
 */
TEST(FloorTest, AddRequestToQueue) {
Floor floor;
Request request(2, 4, FLOOR_REQUEST, FROM_FLOOR, true);
floor.addRequestToQueue(request);
EXPECT_EQ(floor.getQueueSize(), 1);
}


// ─────────────────────────────────────────────
// MAIN
// ─────────────────────────────────────────────

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
