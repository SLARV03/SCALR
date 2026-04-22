cmake_minimum_required(VERSION 3.10)
project(SCALR)

# Set C++ standard to C++20 as requested in the original Makefile
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find all source files recursively
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Define the executable target "scalr"
# This matches the binary name expected by scalr_gui2.py
add_executable(scalr ${SOURCES})

# Specify the include directory
target_include_directories(scalr PRIVATE include)

# Configure output directory directly into the project root
# This matches scalr_gui2.py's expectation to find the backend alongside the python script
set_target_properties(scalr PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Add compiler flags
if(MSVC)
    target_compile_options(scalr PRIVATE /W3)
else()
    target_compile_options(scalr PRIVATE -Wall -Wextra)
endif()
