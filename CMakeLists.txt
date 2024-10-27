# Project setup
cmake_minimum_required(VERSION 3.16)
project(aray C)

# Profiles
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message("Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
endif()

# Creates the compile_commands.json file for your LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Create symlink for compile_commands.json in root dir for lsp
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
)

# Output path
set(OutputDirectory "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OutputDirectory}/obj")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OutputDirectory}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OutputDirectory}")

# Add custom includes
include_directories(include)

# Add the subdirectories
add_subdirectory(src)
add_subdirectory(test)
