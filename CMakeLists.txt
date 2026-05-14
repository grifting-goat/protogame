cmake_minimum_required(VERSION 3.15)

project(ProtoGame LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Export compile commands for IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Output to bin folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Use vendored SDL3
add_subdirectory(libs/SDL3 EXCLUDE_FROM_ALL)

# Use vendored SDL3_ttf
set(SDLTTF_VENDORED ON)
add_subdirectory(libs/SDL3_ttf EXCLUDE_FROM_ALL)

add_definitions(-DSDL_MAIN_HANDLED)

# Collect all source files from src directory
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.c")

# Create executable
add_executable(protogame ${SOURCES})

# Find OpenGL and link to SDL3, SDL3_ttf
find_package(OpenGL REQUIRED)

# Link libraries
if(WIN32)
    target_link_libraries(protogame PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf ${OPENGL_LIBRARIES} user32 gdi32 winmm imm32 ole32 oleaut32 version uuid)
else()
    target_link_libraries(protogame PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf ${OPENGL_LIBRARIES})
endif()