# README for SYSC 3303 Project Iteration 3:Multiple Cars

## Overview
This project simulates an elevator control system with three subsystems:
1. Floor Subsystem
   - The Floor Subsystem handles user requests and sends them to the Scheduler.
2. Scheduler
   - The Scheduler assigns an elevator to the requests and manages system coordination.
3. Elevator Subsystem
   - The Elevator Subsystem processes assigned requests and executes the movement of the elevator.
   -Notifies the scheduler upon reaching a requested floor.

For Iteration 3, we expand the system by introducing multiple elevators and distributing the system across three or more computers. The subsystems will communicate using UDP, leveraging concepts from Assignments 2 and 3. 

## Goals  
- System Distribution: Each subsystem runs on separate machines and communicates over UDP.
- Multi-Elevator Coordination: The scheduler balances elevator loads to minimize wait times.
-Independent Elevator Execution: Each elevator operates autonomously but shares its status with the scheduler.
-Intelligent Request Handling: The scheduler assigns requests efficiently based on elevator position and direction.

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
1) g++ -DTEST_MODE -o elevator_tests unit_test.cpp Elevator.cpp Scheduler.cpp Floor.cpp -pthread -std=c++11 -lgtest -lgtest_main
2) ./elevator_tests