cmake_minimum_required(VERSION 3.16)
project(CPU_Usage_Tracker C)

set(CMAKE_C_STANDARD 11)
add_executable(CPU_Usage_Tracker main.c)

string(FIND "${CMAKE_C_COMPILER}" "clang" MAK_COMPILER_CLANG)
if(MAK_COMPILER_CLANG GREATER -1)
    target_compile_options(CPU_Usage_Tracker PRIVATE -Weverything -Wno-atomic-implicit-seq-cst)
    target_link_options(CPU_Usage_Tracker PRIVATE -lpthread)
else()
    target_compile_options(CPU_Usage_Tracker PRIVATE -Wall -Wextra --pedantic -Wno-atomic-implicit-seq-cst)
    target_link_options(CPU_Usage_Tracker PRIVATE -pthread)
endif()

