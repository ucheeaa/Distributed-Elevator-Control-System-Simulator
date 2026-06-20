# README for SYSC 3303 Project Iteration 5: Capacity Limits and User Interface

## Overview
This project simulates an elevator control system with three subsystems:
1. Floor Subsystem
   - The Floor Subsystem handles user requests and sends them to the Scheduler.
2. Scheduler
   - The Scheduler assigns an elevator to the requests and manages system coordination.
3. Elevator Subsystem
   - The Elevator Subsystem processes assigned requests and executes the movement of the elevator.
   -Notifies the scheduler upon reaching a requested floor.

## Iteration 5: Capacity Limits & User Interface
In this final iteration, we enhance the system with:

- Real-Time Display Console: A UI showing the live status of all elevators, their locations, and any faults. This serves as an operator panel for building staff.

- Capacity Limits: Elevators now have a maximum passenger capacity. If an elevator is full, passengers must wait for the next available one. The simulator ensures these passengers "press the button again" automatically.

- Performance Metrics: The system records:

   - Total time required to move all passengers.

   - Number of elevator movements for efficiency analysis.

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

- Elevator: Nethul, Janice
- Floor: Deborah, Janice
- Scheduler: Uchenna and Divya
- Unit Test: Divya
- Convert input file to class: Janice
- Readme: Deborah
- Diagrams review: Uchenna

### Iteration 4:

- Elevator: Janice
- Floor: Deborah
- Scheduler: Uchenna and Divya
- Unit Test: Uchenna
- Readme: Deborah
- Diagrams review: Uchenna
- Timing Diagram: Nethul

### Iteration 5:
 
- Update Sequence Diagram:	Uchenna
- Fix Elevator States & Update State Diagram:	Janice
- Update Class Diagram:	Deborah
- Update README:	Deborah
- Performance Metrics in README:	Deborah
- Update Timing Diagram:	Nethul
- Code Cleanup & Comments:	Divya
- UI Console Display Formatting:	Divya
- Implement Capacity Limits:	Uchenna
- Unit & Acceptance Testing:	Uchenna
- Integration Testing:	Nethul
  
## Compilation Instructions
You will use 3 terminals to run

1) In terminal 1,run :
   g++ -std=c++11 -o scheduler Scheduler.cpp
   
   g++ -std=c++11 -o elevator Elevator.cpp
   
   g++ -std=c++11 -o floor Floor.cpp

   ./scheduler
   
2) In terminal 2, run: ./elevator

3) In terminal 3, run: ./floor


## Unit Test Instructions
1) g++ -DTEST_MODE -o elevator_tests unit_test.cpp Elevator.cpp Scheduler.cpp Floor.cpp -pthread -std=c++14 -lgtest -lgtest_main

2) ./elevator_tests

## Integration Test Instructions
1) g++ -DTEST_MODE -o integration_tests IntegrationTest.cpp Elevator.cpp Scheduler.cpp Floor.cpp -pthread -std=c++14 -lgtest -lgtest_main

2) ./integration_tests

## Acceptance Testing

### Acceptance Test Case – High Load, Fault Injection, Retry Logic

We tested the full elevator system using an input file with 26 requests all initially at Floor 1 with destinations from 5–22. The system simulated:

1) Load balancing across multiple elevators

2) Capacity-based retries when elevators were full

3) Transient and hard fault injections

4) Retry messages and proper reassignments

The system passed: all valid passengers were eventually picked up and dropped off, retries were handled successfully, and faulty elevators were excluded from assignment.

## Estimate of the time it takes the system to handle the input file.
Trial	            Requests	            Movements	            Time (s)	                  

1	                     5	                  39	                  180                  	

2	                     5	                  39	                  180                       	

3	                     5	                  39	                  180                        	

4                     	 5	                  39                   	   180                       

5                        5	                  39         	           180            	      

### Calculate the Mean Time (𝜇):
The mean time (𝜇) is calculated as: 𝜇 = ∑𝑋𝑖 / 𝑛

Mean Time(Average across all rounds)
∑Xi = 180 + 180 + 180 + 180 + 180  = 900
𝑛 = 5
𝜇 = 900/5 = 180s

Mean Time per Movement = 180/39 = 4.615 s

### Where the Instrumentation was added
In this iteration, we instrumented the Elevator class to track and analyze its operational performance, specifically focusing on:

1.  Total Movement Tracking
Variable Used: totalMovements (incremented every time the elevator moves one floor).

Where: Inside simulateMoving() loop after each floor is passed.

Purpose: Quantify total floor-to-floor transitions for each elevator.

2.  Time Measurement
   
-Start Time: Tracked using:

startTime = std::chrono::steady_clock::now() during elevator initialization.

-End Time: Captured at the end of servicing a request via:

std::chrono::steady_clock::now() in sendResponseToScheduler().

-Elapsed Time Calculation: 

duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();

Purpose: Report how long an elevator has been running since it started.

What the Value Means:
This value represents the cumulative operational time (in seconds) that the elevator has been running. It helps contextualize performance metrics, such as requests handled per second or total requests serviced during the simulation.


### Mean time per event, derived or measured
In our case, the mean time per event was derived, not directly measured for every individual event. Specifically, the method used was:
Events were timed as a group using a high-resolution timer. The total time was then divided by the number of events to obtain an average or "mean time per event."
This approach was taken because the timer resolution available on the system was not sufficient to measure each event accurately on its own — many individual events completed faster than the timer's smallest measurable interval (insufficient clock resolution). Averaging over a large number of events reduces the effect of measurement error and gives a more reliable estimate of execution time per event.


### Calculate 95% Confidence Interval (CI)
CI = μ ± t x ( σ/sqrt(n)) = 180 +- t x 0 =180
​
This 95% Confidence Interval: [180, 180]seconds
So the mean is highly stable and reliable estimate

## Analysis of time estimates
The analysis was conducted using controlled test inputs and measured by extracting real-time performance metrics from the program’s execution. The following steps were taken:

Step 1: Code Modifications for Performance Tracking
1. The elevator code was modified to track and display:

- The total number of floor movements (totalMovements)

- The total time taken (duration) from the start of the elevator’s operation to request completion

This information was printed in the console output after each request was processed.

Step 2: Test Input Execution
A set of predefined test inputs was used to evaluate the elevator's performance. Some test cases consisted of multiple requests sent to the elevator system, including different pickup and destination floors.

Step 3: Multiple Runs for Consistency
To ensure accuracy, some input set was executed three times, and performance metrics were recorded. This helped analyze variations in response time due to different scheduling scenarios.

Step 4: Data Collection
The key metrics (totalMovements and totalTime) were collected from the console output for each test

The code was instrumented using time markers around the movement execution loop to measure total elapsed time for each round. Five runs were conducted using the same input (5 requests), and each round required 39 movements, taking exactly 180 seconds. As all measurements were identical, the mean execution time per round was 180s, and the mean time per movement was approximately 4.615s. The standard deviation was 0, resulting in a confidence interval of [180, 180] seconds at 95% confidence, confirming the consistency of the measurement. Due to limitations in timing individual events precisely, total time was used and then divided by the number of movements to obtain the mean time per event.


