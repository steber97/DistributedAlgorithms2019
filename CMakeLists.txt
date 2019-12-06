cmake_minimum_required(VERSION 3.15)
project(distributed_algorithms)

set(CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_FLAGS "-pthread")

add_executable(da_proc da_proc.cpp FifoBroadcast.cpp UrBroadcast.cpp BeBroadcast.cpp Link.cpp utilities.cpp LocalCausalBroadcast.cpp graph.cpp)
