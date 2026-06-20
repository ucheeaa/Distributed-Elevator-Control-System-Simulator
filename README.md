# Elevator Control System Simulator

A C++ distributed, multithreaded elevator control system simulator featuring separate Scheduler, Floor, and Elevator subsystems that communicate using datagram-based message passing.

The system models how elevator requests are received, scheduled, assigned, executed, retried, monitored, and tested in a multi-elevator environment. It includes scheduler coordination, elevator state management, capacity-limit handling, fault scenarios, performance instrumentation, and automated unit/integration testing.

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
- **Multithreaded execution model** using `std::thread` to run subsystem listeners, elevator execution, status updates, scheduler processing, and display updates concurrently.
- **Datagram-based message passing** using custom `DatagramSocket` and `DatagramPacket` classes for communication between subsystems.
- **Scheduler-driven request coordination** for assigning elevators to floor requests and tracking elevator availability, direction, state, and fault status.
- **State-machine elevator behavior** for idle, moving, stopped, door/fault, and out-of-service states.
- **Passenger capacity-limit handling** to prevent elevators from exceeding maximum capacity.
- **Retry and reassignment logic** for requests that cannot be completed due to full elevators, unavailable elevators, or hard faults.
- **Fault handling** for transient and hard fault scenarios, including recovery behavior and out-of-service marking.
- **Real-time console display** showing elevator status, location, direction, and fault information.
- **Performance instrumentation** for total movements, elapsed service time, and mean time per movement.
- **Automated testing** using Google Test for unit, integration, and acceptance-level validation.

## My Contributions

This was a team software engineering project. My primary contributions focused on scheduler coordination, distributed system behavior, reliability, testing, and design documentation.

- Designed and implemented core **Scheduler** logic for coordinating requests between the Floor and Elevator subsystems.
- Worked on the project’s **distributed and multithreaded architecture**, where subsystem listeners, elevator execution, scheduler processing, status updates, and display updates run concurrently.
- Implemented **passenger capacity-limit handling** to prevent elevators from exceeding maximum passenger capacity during high-load scenarios.
- Developed **retry and reassignment logic** so requests could be re-queued when elevators were full, unavailable, or affected by hard faults.
- Added and improved **unit, integration, and acceptance tests** using Google Test to validate scheduler behavior, capacity handling, transient faults, hard faults, request completion, and subsystem interactions.
- Contributed to debugging and integration across the Scheduler, Floor, and Elevator components to ensure end-to-end request flow worked correctly.
- Updated and reviewed system design documentation, including UML, sequence, timing, and state-machine diagrams across multiple project iterations.
- Supported performance analysis by validating movement counts, elapsed service time, and system behavior under repeated controlled test runs.

## Technologies Used

- **Languages:** C++, C
- **Concurrency:** `std::thread`, `std::mutex`, `std::condition_variable`
- **Communication:** Datagram sockets, message passing, subsystem listeners
- **Testing:** Google Test, unit testing, integration testing, acceptance testing
- **Concepts:** Distributed systems, scheduling, state machines, fault handling, retry logic, system simulation, performance instrumentation
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

This project was developed across five course iterations. All five development stages are preserved under the `iterations/` directory for reference. The final cleaned implementation is organized separately in the main `src/`, `include/`, and `tests/` folders.

## Final Report

The final project report is available here:

[View final report](docs/final-report.pdf)
