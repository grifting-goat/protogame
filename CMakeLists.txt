cmake_minimum_required(VERSION 3.15)

project(protogame LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Export compile commands for IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output to bin folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# SDL3 setup
set(SDL3_DIR ${CMAKE_SOURCE_DIR}/libs/SDL3)
include_directories(${SDL3_DIR}/include)
link_directories(${SDL3_DIR}/lib/x64)
add_definitions(-DSDL_MAIN_HANDLED)

# Collect all source files from src directory
file(GLOB_RECURSE SOURCES "src/*.c")

# Create executable
add_executable(protogame ${SOURCES})

# Find Vulkan
if(NOT DEFINED VULKAN_SDK)
    set(VULKAN_SDK "C:/VulkanSDK/1.4.341.1")
endif()

set(Vulkan_INCLUDE_DIR "${VULKAN_SDK}/Include")
set(Vulkan_LIBRARY "${VULKAN_SDK}/Lib/vulkan-1.lib")

include_directories(${Vulkan_INCLUDE_DIR})
link_directories("${VULKAN_SDK}/Lib")

# Link libraries
if(WIN32)
    target_link_libraries(protogame SDL3 ${Vulkan_LIBRARY} user32 gdi32 winmm imm32 ole32 oleaut32 version uuid)
    add_custom_command(TARGET protogame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_DIR}/lib/x64/SDL3.dll"
        $<TARGET_FILE_DIR:protogame>)
else()
    target_link_libraries(protogame SDL3 ${Vulkan_LIBRARY})
endif()
