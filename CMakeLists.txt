cmake_minimum_required(VERSION 3.9)
project(TractoVisualization)

# Module dependencies
find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads)

set(CMAKE_CXX_STANDARD 11)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE lib/glm)

# Targets
add_subdirectory(lib/glad)
add_subdirectory(qfib)
add_subdirectory(visualization)
