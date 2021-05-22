cmake_minimum_required(VERSION 3.19)
project(c_vm_c C)

set(CMAKE_C_STANDARD 11)

add_executable(c_vm_c main.c elf.c elf.h)