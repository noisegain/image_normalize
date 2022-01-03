cmake_minimum_required(VERSION 3.16)
project(skakov)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-fopenmp -O3 -mfma")
#set(CMAKE_CXX_FLAGS -O3)

add_executable(skakov main.cpp)
