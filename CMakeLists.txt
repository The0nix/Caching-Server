cmake_minimum_required(VERSION 3.0)

project(caser)

set(CMAKE_C_FLAGS "-std=c99")
set(CMAKE_C_FLAGS_DEBUG "-std=c99 -g")
set(CMAKE_C_STANDARD 99)

set(LIB_UTILS utils.c)
set(LIB_CONTAINERS containers.c)
add_library(utils ${LIB_UTILS})
# add_library(containers ${LIB_CONTAINERS})
set(LIBS utils)
# set(LIBS ${LIBS} containers) 
set(SOURCE server.c)

set(PNAME caser)
add_executable(${PNAME} ${SOURCE})
target_link_libraries(${PNAME} ${LIBS})
