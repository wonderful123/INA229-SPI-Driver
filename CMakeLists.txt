cmake_minimum_required(VERSION 3.14)
project(INA229 LANGUAGES CXX)

# Create the library
add_library(ina229_lib src/INA229.cpp)

# Define which folders hold the headers
target_include_directories(ina229_lib PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Enforce C++20 (required for your designated initializers)
target_compile_features(ina229_lib PUBLIC cxx_std_20)

# Make it easier for others to use
add_library(INA229::Driver ALIAS ina229_lib)