cmake_minimum_required(VERSION 3.22)
project(magnetic_vm_runtime)

add_library(magnetic_vm_runtime STATIC
        src/strings.cc
        src/strings.h)

set_property(TARGET magnetic_vm_runtime PROPERTY CMAKE_CXX_STANDARD 17)
set_property(TARGET magnetic_vm_runtime PROPERTY CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}")

target_include_directories(magnetic_vm_runtime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
