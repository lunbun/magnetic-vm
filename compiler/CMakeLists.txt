cmake_minimum_required(VERSION 3.22)
project(magnetic_vm)

add_executable(magnetic_vm src/main.cc)

set_property(TARGET magnetic_vm PROPERTY CMAKE_CXX_STANDARD 17)
set_property(TARGET magnetic_vm PROPERTY CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}")

find_package(CJBP REQUIRED)
find_package(fmt REQUIRED)
find_package(zip REQUIRED)

find_package(LLVM REQUIRED CONFIG)
if ("${LLVM_PACKAGE_VERSION}" VERSION_LESS "14.0.0")
    message(FATAL_ERROR "LLVM must be version 14.0.0 or greater")
endif()
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
target_include_directories(magnetic_vm PRIVATE ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
llvm_map_components_to_libnames(LLVM_LIBS support core passes)

target_include_directories(magnetic_vm PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_subdirectory(src)

target_link_libraries(magnetic_vm cjbp::cjbp fmt::fmt zip::zip ${LLVM_LIBS})
