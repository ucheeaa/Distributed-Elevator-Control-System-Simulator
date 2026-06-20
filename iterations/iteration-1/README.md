# README for SYSC 3303 Project Iteration 1: Establish Connections between the three subsystems

## Overview
This project is part of an iterative development of an elevator control system. It includes implementing core system components, such as elevator movement, floor requests, and scheduling. The system models how an elevator handles requests and selects destinations efficiently. This project simulates an elevator control system with three subsystems:
1. Floor Subsystem
2. Scheduler
3. Elevator Subsystem
The Floor Subsystem handles user requests and sends them to the Scheduler.
The Scheduler assigns an elevator to the requests and manages system coordination.
The Elevator Subsystem processes assigned requests and executes the movement of the elevator.

## File Structure
- main.cpp - the entry point of the program.
- scheduler.cpp - implements the scheduler logic.
- scheduler.h - defines the scheduler class and its methods.
- floor.cpp - implements the floor subsystem functionality.
- floor.h - defines the floor subsystem structure.
- elevator.cpp - implements elevator functionality.
- elevator.h - defines the elevator class and its methods.

## Compilation Instructions
# Requirements
- c++ compiler (e.g., g++, clang++)
- c++ standard library
To compile the project using g++, run: g++ -o elevator_system main.cpp elevator.cpp floor.cpp scheduler.cpp
# Running the program. 
Execute the compiled binary: ./elevator_system
if input.txt is required, you can provide it as input: ./elevator_system < input.txt

## UML Diagrams
- elevatorsubsystem_class.jpg - uml class diagram for the elevator subsystem.
- requestelevator_sequence.jpg - sequence diagram illustrating the request handling process.
- selectdestination_sequence.jpg - sequence diagram illustrating how destinations are selected.

## Other Files
- input.txt - sample input file for testing elevator requests.
- readme.md - this document.

## Known Issues & Future Enhancements
- Simulation completion notification: the current implementation processes all requests correctly, but does not explicitly notify when the simulation has fully completed. you must manually stop the program using ctrl + c after all requests have been processed.
- Planned enhancement: in the next iteration, we will introduce an explicit "simulation complete" message and ensure proper termination of all threads

