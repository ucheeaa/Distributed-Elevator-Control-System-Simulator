# Elevator Control System Simulator

A C++ distributed elevator control system simulator featuring separate Scheduler, Floor, and Elevator subsystems, capacity-limit handling, fault scenarios, performance tracking, and automated testing.

This project models how elevator requests are received, scheduled, assigned, executed, retried, and monitored in a multi-elevator system.

## Demo Video

A demonstration video of the elevator control system simulator is available here:

[Watch the demo video](https://drive.google.com/file/d/1s_gr5KxuRLOvAqScu-tjrPZsZd-Ao8R8/view?usp=sharing)

## Overview

The system is divided into three main subsystems:

1. **Floor Subsystem**  
   Handles user floor requests and sends them to the Scheduler.

2. **Scheduler**  
   Coordinates the system by assigning elevators to requests, managing retries, and handling request scheduling.

3. **Elevator Subsystem**  
   Processes assigned requests, simulates elevator movement, updates elevator state, and notifies the Scheduler when requested floors are reached.

## Key Features

- **Distributed subsystem architecture** with separate Floor, Scheduler, and Elevator components.
- **Real-time console display** showing elevator status, location, direction, and fault information.
- **Passenger capacity limits** to prevent elevators from accepting more passengers than allowed.
- **Retry logic** that allows passengers to wait and re-request service when elevators are full.
- **Fault handling** for transient and hard fault scenarios.
- **Performance metrics** including total elevator movements and total service time.
- **Unit, integration, and acceptance testing** using Google Test.

## My Contributions

This was a team software engineering project. My main contributions included:

- Implemented Scheduler logic for assigning elevators to floor requests.
- Implemented passenger capacity-limit handling and retry logic when elevators became full.
- Added unit and acceptance tests for scheduler behavior, capacity handling, fault scenarios, and request reassignment.
- Updated and reviewed UML, sequence, and system diagrams across multiple iterations.
- Contributed to debugging and integration across the Floor, Scheduler, and Elevator subsystems.

## Technologies Used

- **Languages:** C++, C
- **Testing:** Google Test
- **Concepts:** Distributed systems, scheduling, state machines, fault handling, system simulation, performance instrumentation
- **Tools:** Git, Linux/macOS terminal, g++

## Repository Structure

```text
Distributed-Elevator-Control-System-Simulator/
├── src/                  # Final cleaned source code
├── include/              # Header files
├── tests/                # Unit and integration tests
├── docs/                 # Diagrams, reports, and supporting documentation
├── iterations/           # Archived course iterations
│   ├── iteration-1/
│   ├── iteration-2/
│   ├── iteration-3/
│   ├── iteration-4/
│   └── iteration-5/
└── README.md
```

## Build and Run Instructions

The simulator runs as three separate programs: Scheduler, Elevator, and Floor.

Compile the components:

~~~bash
g++ -std=c++14 -Iinclude src/Scheduler.cpp -o scheduler
g++ -std=c++14 -Iinclude src/Elevator.cpp -o elevator
g++ -std=c++14 -Iinclude src/Floor.cpp -o floor
~~~

Run the system using three terminals:

**Terminal 1:**

~~~bash
./scheduler
~~~

**Terminal 2:**

~~~bash
./elevator
~~~

**Terminal 3:**

~~~bash
./floor
~~~

## Testing

Compile and run the unit tests:

~~~bash
g++ -DTEST_MODE -Iinclude -o elevator_tests tests/unit_test.cpp src/Elevator.cpp src/Scheduler.cpp src/Floor.cpp -pthread -std=c++14 -lgtest -lgtest_main
./elevator_tests
~~~

Compile and run the integration tests:

~~~bash
g++ -DTEST_MODE -Iinclude -o integration_tests tests/IntegrationTest.cpp src/Elevator.cpp src/Scheduler.cpp src/Floor.cpp -pthread -std=c++14 -lgtest -lgtest_main
./integration_tests
~~~

## Acceptance Testing

The final system was tested using a high-load input scenario with 26 requests starting from Floor 1 and destinations ranging from Floors 5 to 22.

The acceptance test covered:

- Load balancing across multiple elevators.
- Capacity-based retries when elevators reached maximum capacity.
- Transient and hard fault scenarios.
- Reassignment of requests away from faulty elevators.
- Successful pickup and drop-off of valid passengers.

The system passed the acceptance test: valid passengers were eventually picked up and dropped off, retry logic worked successfully, and faulty elevators were excluded from future assignments.

## Performance Metrics

The system was instrumented to track elevator movement and service time.

Metrics collected included:

- Total number of elevator movements.
- Total elapsed service time.
- Mean time per elevator movement.

For one controlled test scenario with 5 requests:

| Trial | Requests | Movements | Time (s) |
|------:|---------:|----------:|---------:|
| 1 | 5 | 39 | 180 |
| 2 | 5 | 39 | 180 |
| 3 | 5 | 39 | 180 |
| 4 | 5 | 39 | 180 |
| 5 | 5 | 39 | 180 |

Average service time:

~~~text
Mean time = 180 seconds
Mean time per movement = 180 / 39 = 4.615 seconds
~~~

Because all five runs produced the same measured time, the standard deviation was 0 for this controlled scenario.

## Instrumentation Details

Performance instrumentation was added to the Elevator class.

- `totalMovements` was incremented each time an elevator moved one floor.
- `startTime` was recorded during elevator initialization using `std::chrono::steady_clock::now()`.
- `endTime` was recorded after servicing a request.
- Elapsed time was calculated using `std::chrono::duration_cast<std::chrono::seconds>()`.

This allowed the system to report cumulative runtime and movement count during simulation.

## Archived Iterations

This project was developed across five course iterations. All five development iterations are preserved under the `iterations/` directory for reference. The final cleaned implementation is organized separately in `src/`, `include/`, and `tests/`.

## Final Report

The final project report is available here:

[View final report](docs/final-report.pdf)
