#include "Scheduler.h"
#include "Elevator.h"
#include "Floor.h"
#include <iostream>
#include <thread>
#include <vector>

int main() {
    // Create elevator instance
    Elevator* elevator = new Elevator(0);
    std::vector<Elevator*> elevators = { elevator };

    // Create scheduler instance
    Scheduler scheduler(elevators, nullptr);

    // Create floor subsystem
    Floor floorSubsystem(&scheduler);
    scheduler.setFloorSubsystem(&floorSubsystem);

    // Start floor processing
    std::string inputFilename = "input.txt";
    std::thread floorThread(&Floor::start, &floorSubsystem, inputFilename);

    // Start scheduler
    std::thread schedulerThread(&Scheduler::processRequests, &scheduler);

    // Start elevators
    std::vector<std::thread> elevatorThreads;
    for (Elevator* e : elevators) {
        elevatorThreads.emplace_back(&Elevator::run, e, &scheduler);
    }

    // Let the simulation run for a few seconds
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Stop all processes in order
    std::cout << "Stopping Floor Subsystem...\n";
    floorSubsystem.stop();
    if (floorThread.joinable()) floorThread.join();

    std::cout << "Stopping Scheduler...\n";
    scheduler.stop();
    if (schedulerThread.joinable()) schedulerThread.join();

    std::cout << "Stopping Elevators...\n";
    for (Elevator* e : elevators) {
        e->stop();
    }

    for (std::thread& t : elevatorThreads) {
        if (t.joinable()) t.join();
    }

    // Cleanup allocated memory
    for (Elevator* e : elevators) {
        delete e;
    }

    std::cout << "Simulation complete!\n";
    return 0;
}
