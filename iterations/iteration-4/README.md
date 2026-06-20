# README for SYSC 3303 Project Iteration 4:Error Detection and Correction

## Overview
This project simulates an elevator control system with three subsystems:
1. Floor Subsystem
   - The Floor Subsystem handles user requests and sends them to the Scheduler.
2. Scheduler
   - The Scheduler assigns an elevator to the requests and manages system coordination.
3. Elevator Subsystem
   - The Elevator Subsystem processes assigned requests and executes the movement of the elevator.
   -Notifies the scheduler upon reaching a requested floor.

For Iteration 3, the system was expanded to support multiple elevators and distributed execution across three or more computers, using UDP communication.

For Iteration 4, we introduce error detection and correction mechanisms to handle elevator faults, such as:

-Stuck elevators: If an elevator does not reach a floor within a set time, it is considered stuck.

-Sensor failures: If an arrival sensor fails, the scheduler must handle the missing signal.

-Door malfunctions: The system must detect whether a door fails to open or remains stuck open.

-Graceful fault handling: The system must recover from transient faults (like a stuck door). However, a floor timer failure should be treated as a hard fault, shutting down the affected elevator.

## Goals  
-Fault Detection: Implement timers to detect when an elevator takes too long to reach a floor.

-Fault Injection: Modify the input file format to allow injecting faults into the system for testing.

-Fault Handling:

   -If an elevator is stuck, the system should reassign its passengers to another elevator.

   -If a door is stuck open, the system should attempt to close it again before shutting down the elevator.

## File Structure
- **Scheduler.cpp**: Implements the scheduler logic, including request handling and decision-making for elevator movement.  
- **Scheduler.h**: Declares the scheduler class, including function prototypes and data structures used for scheduling.  
- **Floor.cpp**: Implements the floor subsystem, sends requests to the scheduler. 
- **Floor.h**: Defines the floor subsystem structure, including relevant data types and function declarations.  
- **Elevator.cpp**: Implements elevator functionality
- **Elevator.h**: Declares the elevator class, defining its state machine, control functions, and interactions with the scheduler.  

### Responsibilities of Each Team Member 
### Iteration 1:
- READMME: Nethul
- Diagrams: Janice
- Elevator subsystem:Divya
- Scheduler: Uchenna
- Floor subsystem: Deborah
  
### Iteration 2:
- Divya, Deborah, Uchenna fix the old code (stop issue and others)
- New functionalities yet to be implemented: Nethul and Janice
- Readme - Deborah
- UML diagrams - Uchenna
- State Diagrams - Divya

### Iteration 3:

-Elevator: Nethul, Janice
-Floor: Deborah, Janice
-Scheduler: Uchenna and Divya
-Unit Test: Divya
-Convert input file to class: Janice
-Readme: Deborah
-Diagrams review: Uchenna

### Iteration 4:

-Elevator: Janice
-Floor: Deborah
-Scheduler: Uchenna and Divya
-Unit Test: Uchenna
-Readme: Deborah
-Diagrams review: Uchenna
-Timing Diagram: Nethul
  
## Compilation Instructions
You will use 3 terminals to compile
1) In terminal 1,run :
   g++ -std=c++11 -o scheduler Scheduler.cpp
   
   g++ -std=c++11 -o elevator Elevator.cpp
   
   g++ -std=c++11 -o floor Floor.cpp

   ./scheduler
   
2) In terminal 2, run: ./elevator

3) In terminal 3, run: ./floor


## Unit Test Instructions
1) g++ -DTEST_MODE -o elevator_tests unit_test.cpp Elevator.cpp Scheduler.cpp Floor.cpp -pthread -std=c++11 -lgtest -lgtest_main -lgmock -lgmock_main

2) ./elevator_tests
