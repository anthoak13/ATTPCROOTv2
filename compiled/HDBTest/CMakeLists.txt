# CMakeLists.txt for event package. It creates a library with dictionary and a main program
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(HDBTest)

SET(ATTPCROOTPATH $ENV{VMCWORKDIR})

list(APPEND CMAKE_PREFIX_PATH ${ATTPCROOTPATH}/build/install) #Need access to ATTPCROOT

message(STATUS "Prefix path is: ${CMAKE_PREFIX_PATH}")
# Will also load all of its dependecies
find_package(ATTPCROOT 0.3 REQUIRED)

set(CMAKE_MODULE_PATH "${ATTPCROOTPATH}/build/install/lib/cmake/ATTPCROOT/modules")

include(ROOTTargetMacros)
include(GNUInstallDirs)
set(CMAKE_BUILD_TYPE RELWITHDEBINFO)
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(HDBSCAN-CPP)

add_executable(HDBTest HDBSCAN-FourProminentClusterExample/FourProminentClusterExample.cpp) 
target_link_libraries(HDBTest HDBSCAN)

add_executable(EventTest ClusterEvent.cpp) 
target_link_libraries(EventTest HDBSCAN)