cmake_minimum_required(VERSION 3.16)
project(fluid2_host_config_regression LANGUAGES C CXX)
enable_testing()

# Model the application's directory-wide includes and identically named header.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/config.h"
    "#error Host config.h leaked into bundled FluidSynth\n")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
set(MSCORE_FLUIDSYNTH2_PROVIDER BUNDLED)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../../../build/FluidSynth2.cmake")
mscore_setup_fluidsynth2()

add_executable(tst_fluid2_host_config main.cpp)
target_link_libraries(tst_fluid2_host_config PRIVATE MScore::FluidSynth2)
add_test(NAME tst_fluid2_host_config COMMAND tst_fluid2_host_config)
