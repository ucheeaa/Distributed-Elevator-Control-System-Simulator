# README for SYSC 3303 Project Iteration 2: Implement state machines for both the **scheduler** and **elevator** subsystems.

## Overview
This project simulates an elevator control system with three subsystems:
1. Floor Subsystem
   - The Floor Subsystem handles user requests and sends them to the Scheduler.
2. Scheduler
   - The Scheduler assigns an elevator to the requests and manages system coordination.
3. Elevator Subsystem
   - The Elevator Subsystem processes assigned requests and executes the movement of the elevator.
   -Notifies the scheduler upon reaching a requested floor.

In this iteration, we build on the **scheduler** and **elevator subsystems**, focusing on the interaction between them. The system currently assumes **only one elevator**, but the design should consider future expansion to handle multiple elevators efficiently.  

## Goals  
- Implement state machines for both the **scheduler** and **elevator** subsystems.  
- Ensure the **scheduler** receives and processes floor requests.  
- Enable the **elevator subsystem** to notify the scheduler when an elevator reaches a floor.  
- Lay the groundwork for **multi-elevator coordination**, which will be addressed in the next iteration.

## File Structure
- **Scheduler.cpp**: Implements the scheduler logic, including request handling and decision-making for elevator movement.  
- **Scheduler.h**: Declares the scheduler class, including function prototypes and data structures used for scheduling.  
- **Floor.cpp**: Implements the floor subsystem, sends requests to the scheduler. 
- **Floor.h**: Defines the floor subsystem structure, including relevant data types and function declarations.  
- **Elevator.cpp**: Implements elevator functionality
- **Elevator.h**: Declares the elevator class, defining its state machine, control functions, and interactions with the scheduler.  

### Responsibilities of Each Team Member 
- Divya, Deborah, Uchenna fix the old code (stop issue and others)
- New functionalities yet to be implemented: Nethul and Janice
- Readme - Deborah
- UML diagrams - Uchenna
- State Diagrams - Divya
  
## Compilation Instructions
1) what you use to run: g++ -std=c++11 -o elevator_system main.cpp Floor.cpp Elevator.cpp Scheduler.cpp
   
2)./elevator_system   

