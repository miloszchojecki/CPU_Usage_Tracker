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

add_executable(cpu src/main.c src/readCpuStats.c)
add_executable(test tests/test.c src/readCpuStats.c)

target_include_directories(cpu PUBLIC src)
target_include_directories(test PUBLIC src)

target_link_libraries(cpu PRIVATE Threads::Threads)