cmake_minimum_required(VERSION 3.30)

project(
    Yinsh-gui
    VERSION 0.0.0
    LANGUAGES CXX C
)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(extern)
add_subdirectory(yinsh-gui)
