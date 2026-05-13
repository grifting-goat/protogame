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

# SDL3 static linking
set(SDL3_DIR ${CMAKE_SOURCE_DIR}/libs/SDL3)
include_directories(${SDL3_DIR}/include)
link_directories(${SDL3_DIR}/lib/x64)
add_definitions(-DSDL_MAIN_HANDLED)

# Collect all source files from src directory
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.c")

# Create executable
add_executable(protogame ${SOURCES})

# Find OpenGL
find_package(OpenGL REQUIRED)

# Link SDL3 statically and OpenGL
if(WIN32)
    target_link_libraries(protogame SDL3 ${OPENGL_LIBRARIES} user32 gdi32 winmm imm32 ole32 oleaut32 version uuid)
    # Copy SDL3.dll to output directory
    add_custom_command(TARGET protogame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_DIR}/lib/x64/SDL3.dll"
        $<TARGET_FILE_DIR:protogame>)
else()
    target_link_libraries(protogame SDL3 ${OPENGL_LIBRARIES})
endif()