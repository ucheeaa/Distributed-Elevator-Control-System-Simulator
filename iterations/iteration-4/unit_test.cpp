#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Elevator.h"
#include "Scheduler.h"
#include "Floor.h"
#include "Datagram.h"

#define FLOOR_NOTIFICATION_PORT 50002


// -------------------- ELEVATOR TESTS --------------------
TEST(ElevatorTest, Initialization) {
Elevator elevator(1);
EXPECT_EQ(elevator.getId(), 1);
EXPECT_EQ(elevator.getCurrentFloor(), 0);
EXPECT_TRUE(elevator.isIdle());
}

TEST(ElevatorTest, AddRequestSetsNonIdle) {
Elevator elevator(1);
Request request(1, 5, FLOOR_REQUEST, FROM_FLOOR, true);
elevator.addRequest(request);
EXPECT_FALSE(elevator.isIdle());
}

TEST(ElevatorTest, SimulateMovementUpdatesCurrentFloor) {
Elevator elevator(1);
elevator.setCurrentFloor(1);
elevator.simulateMoving(3);
EXPECT_EQ(elevator.getCurrentFloor(), 3);
}

// -------------------- FLOOR TESTS --------------------
TEST(FloorTest, AddRequestToQueue) {
Floor floor;
Request request(2, 4, FLOOR_REQUEST, FROM_FLOOR, true);
floor.addRequestToQueue(request);
EXPECT_EQ(floor.getQueueSize(), 1);
}

// -------------------- SCHEDULER TESTS --------------------
TEST(SchedulerTest, AddRequestUpQueue) {
Scheduler scheduler;
scheduler.addRequest(3, 6, FLOOR_REQUEST, FROM_FLOOR, true);
EXPECT_EQ(scheduler.getUpQueue().size(), 1);
EXPECT_EQ(scheduler.getUpQueue().front().floor, 3);
}

TEST(SchedulerTest, AddRequestDownQueue) {
Scheduler scheduler;
scheduler.addRequest(7, 2, FLOOR_REQUEST, FROM_FLOOR, false);
EXPECT_EQ(scheduler.getDownQueue().size(), 1);
EXPECT_EQ(scheduler.getDownQueue().front().floor, 7);
}

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

// -------------------- FAULT DETECTION (with IDLE skip) --------------------
TEST(SchedulerTest, TimeoutIgnoresIdleElevator) {
Scheduler scheduler;
auto now = std::chrono::steady_clock::now();

scheduler.getElevatorLastUpdate()[1] = now - std::chrono::seconds(20);
scheduler.getElevatorFaults()[1] = false;
elevatorStatuses[1] = { 3, ElevatorState::IDLE };

// Simulate logic
for (auto& entry : scheduler.getElevatorLastUpdate()) {
int elevatorId = entry.first;
auto lastUpdate = entry.second;

if (!scheduler.getElevatorFaults()[elevatorId] &&
elevatorStatuses[elevatorId].state != ElevatorState::IDLE &&
        std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count() > 15) {
scheduler.getElevatorFaults()[elevatorId] = true;
}
}

EXPECT_FALSE(scheduler.getElevatorFaults()[1]);  // should not mark as faulty
}

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

// -------------------- MOCK notifyFloor() --------------------

TEST(SchedulerTest, NotifyFloorActuallySendsDataToPort) {
// Arrange: Start a receiver socket on the expected port
DatagramSocket receiver(FLOOR_NOTIFICATION_PORT);

// Run listener in a separate thread
std::thread receiveThread([&]() {
    std::vector<uint8_t> buffer(4 * sizeof(int));
    DatagramPacket received(buffer, buffer.size());
    try {
        receiver.receive(received);
        int elevatorId, origin, pickup, dest;
        memcpy(&elevatorId, buffer.data(), sizeof(int));
        memcpy(&origin, buffer.data() + sizeof(int), sizeof(int));
        memcpy(&pickup, buffer.data() + 2 * sizeof(int), sizeof(int));
        memcpy(&dest, buffer.data() + 3 * sizeof(int), sizeof(int));

        EXPECT_EQ(elevatorId, 5);
        EXPECT_EQ(origin, 1);
        EXPECT_EQ(pickup, 2);
        EXPECT_EQ(dest, 7);
    } catch (...) {
        FAIL() << "Did not receive packet.";
    }
});

std::this_thread::sleep_for(std::chrono::milliseconds(100)); // give time to bind

Scheduler scheduler;
scheduler.notifyFloor(5, 1, 2, 7);

receiveThread.join();
}

// -------------------- ELEVATOR FAULT TESTS --------------------

TEST(ElevatorTest, HandlesTransientDoorStuckFault) {
Elevator elevator(1);

Request request(1, 3, FLOOR_REQUEST, FROM_FLOOR, true);
request.faultFlag = 1;

elevator.addRequest(request);
std::thread elevatorThread(&Elevator::run, &elevator);

// Let it run enough to complete retries and trip
std::this_thread::sleep_for(std::chrono::seconds(15));

// Clean shutdown for test
elevator.stop();
elevatorThread.join();

// Final check
EXPECT_TRUE(elevator.isIdle());
}

TEST(ElevatorTest, HandlesHardFaultAndShutsDown) {
Elevator elevator(2);
Request request(2, 5, FLOOR_REQUEST, FROM_FLOOR, true);
request.faultFlag = 2;

elevator.addRequest(request);
std::thread elevatorThread(&Elevator::run, &elevator);

elevatorThread.join();

EXPECT_EQ(elevator.getState(), ElevatorState::OUT_OF_SERVICE);
}


TEST(ElevatorTest, HandlesNormalRequestNoFault) {
Elevator elevator(3);

Request request(1, 4, FLOOR_REQUEST, FROM_FLOOR, true);
request.faultFlag = 0;

elevator.addRequest(request);
std::thread elevatorThread(&Elevator::run, &elevator);

std::this_thread::sleep_for(std::chrono::seconds(10));

EXPECT_TRUE(elevator.isIdle());

elevator.stop();
elevatorThread.join();
}

// -------------------- SCHEDULER ASSIGNMENT TEST --------------------

TEST(SchedulerTest, AssignsClosestAndLeastLoadedElevator) {
Scheduler scheduler;

scheduler.getElevatorFloors()[1] = 1;
scheduler.getElevatorFloors()[2] = 3;
scheduler.getElevatorFloors()[3] = 6;

scheduler.getElevatorRequestQueue()[1] = std::queue<Request>(); // 0 load
scheduler.getElevatorRequestQueue()[2].push(Request());         // 1 request
scheduler.getElevatorRequestQueue()[3].push(Request());         // 2 requests
scheduler.getElevatorRequestQueue()[3].push(Request());

Request req(2, 7, FLOOR_REQUEST, FROM_FLOOR, true);
scheduler.assignElevator(req);

// Elevator 1 is closest and has least load → should receive the request
EXPECT_EQ(scheduler.getElevatorRequestQueue()[1].front().floor, 2);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
