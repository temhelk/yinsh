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

add_subdirectory(yinsh-gui)

add_subdirectory(extern/yngine)
target_link_libraries(Yinsh-gui PRIVATE Yngine::Yngine)

add_subdirectory(extern/raylib)
target_link_libraries(Yinsh-gui PRIVATE raylib)

add_subdirectory(extern/raylib-cpp)
target_link_libraries(Yinsh-gui PRIVATE raylib_cpp)

add_library(raygui INTERFACE)
target_include_directories(raygui INTERFACE extern/raygui/src)
target_link_libraries(Yinsh-gui PRIVATE raygui)
