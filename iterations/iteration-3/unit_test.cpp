#include <gtest/gtest.h>
#include "Elevator.h"
#include "Floor.h"
#include "Scheduler.h"

// Test Elevator Initialization
TEST(ElevatorTest, Initialization) {
    Elevator elevator(1);
    EXPECT_EQ(elevator.getId(), 1);
    EXPECT_EQ(elevator.getCurrentFloor(), 0);
    EXPECT_TRUE(elevator.isIdle());
}

// Test Elevator Request Handling
TEST(ElevatorTest, AddRequest) {
    Elevator elevator(1);
    Request request(2, 5, FLOOR_REQUEST, FROM_FLOOR, true);
    elevator.addRequest(request);
    EXPECT_FALSE(elevator.isIdle());
}

// Test Elevator Movement
TEST(ElevatorTest, MoveToFloor) {
    Elevator elevator(1);
    Request request(2, 5, FLOOR_REQUEST, FROM_FLOOR, true);
    elevator.addRequest(request);
    elevator.run();
    EXPECT_EQ(elevator.getCurrentFloor(), 5);
}

// Test Floor Processing Requests
TEST(FloorTest, ProcessRequests) {
    Floor floor;
    Request request(3, 7, FLOOR_REQUEST, FROM_FLOOR, true);
    floor.addRequestToQueue(request);
    EXPECT_NO_THROW(floor.processRequests());
}

// Test Scheduler Assigning Elevators
TEST(SchedulerTest, AssignElevator) {
    Scheduler scheduler;
    Request request(1, 5, FLOOR_REQUEST, FROM_FLOOR, true);
    scheduler.addRequest(request.floor, request.destinationFloor, request.type, request.source, request.goingUp);
    EXPECT_NO_THROW(scheduler.processRequests());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}