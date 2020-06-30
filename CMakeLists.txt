project(fmidi)
cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")

add_library(fmidi SHARED fluid.c)
set_target_properties(fmidi PROPERTIES PREFIX "")
target_link_libraries(fmidi fluidsynth)
