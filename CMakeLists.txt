cmake_minimum_required(VERSION 3.0)
project(CPU_Usage_Tracker)

set(CMAKE_C_COMPILER clang)

if (DEFINED ENV{CC})
    set(CMAKE_C_COMPILER $ENV{CC})
endif()

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Weverything -Wno-atomic-implicit-seq-cst -std=c11")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra --pedantic -Wno-atomic-implicit-seq-cst -std=c11")
endif()

file(GLOB SOURCES "*.c")

find_package(Threads REQUIRED)
if (THREADS_HAVE_PTHREAD_ARG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
endif()

add_executable(cpu ${SOURCES})

target_link_libraries(cpu PRIVATE Threads::Threads)