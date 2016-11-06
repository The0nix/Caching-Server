cmake_minimum_required(VERSION 3.0)

set(CMAKE_C_FLAGS "-std=c99")
set (CMAKE_C_STANDARD 99)
set(SOURCE server.c)

add_executable(server ${SOURCE})
