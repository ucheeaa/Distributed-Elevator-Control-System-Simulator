cmake_minimum_required(VERSION 3.10)
project(SC_3303_Elevator_Project)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add your source files here
add_executable(elevator_project Floor.cpp Scheduler.cpp Elevator.cpp main.cpp)
