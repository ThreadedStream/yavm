cmake_minimum_required(VERSION 3.19)
project(vm_c)

set(CMAKE_CXX_STANDARD 14)

add_executable(vm_c main.c vm.h vm.c instructions.c instructions.h)