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

include(FetchContent)
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.12.0 # Specify the version of pybind11 you want to use
)

FetchContent_MakeAvailable(pybind11)

unset(Python3_EXECUTABLE CACHE)
unset(Python3_INCLUDE_DIR CACHE)
unset(Python3_LIBRARY CACHE)
unset(Python3_FOUND CACHE)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import sklearn.cluster "
    RESULT_VARIABLE EXIT_CODE
    OUTPUT_QUIET
)
if(NOT ${EXIT_CODE} EQUAL 0)
    message(WANRING " Python module sklearn.cluster not found. Skipping python clustering.")
    message( "Python executable: ${Python3_EXECUTABLE}")
else()
    message(STATUS "Python module sklearn.cluster found. Adding python clustering.")
endif()

add_subdirectory(HDBSCAN-CPP)

add_executable(HDBTest HDBSCAN-FourProminentClusterExample/FourProminentClusterExample.cpp) 
target_link_libraries(HDBTest HDBSCAN)

add_executable(EventTest ClusterEvent.cpp) 
target_link_libraries(EventTest HDBSCAN)

add_executable(PythonTest PythonTest.cpp)
target_link_libraries(PythonTest PRIVATE pybind11::embed HDBSCAN)