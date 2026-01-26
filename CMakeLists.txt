# Top-level CMakeLists.txt for Synesthetix ESP-IDF project
cmake_minimum_required(VERSION 3.16)

# Set src as main component directory (PlatformIO uses src/ not main/)
set(EXTRA_COMPONENT_DIRS src)

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(synesthetix)