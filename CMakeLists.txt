cmake_minimum_required(VERSION 3.31.6)

# Project name
project(two_half)

# Allows debug run with: cmake -DCMAKE_BUILD_TYPE=Debug ..
set(CMAKE_BUILD_TYPE Debug)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF) # Stick to standard C++

# Find SFML components
set(SFML_DIR "${CMAKE_SOURCE_DIR}/external/SFML/lib/cmake/SFML")
find_package(SFML 2 COMPONENTS graphics window system audio REQUIRED CONFIG)

# Add source files from engine/ and game/
file(GLOB_RECURSE ENGINE_SOURCES engine/src/*.cpp)
file(GLOB_RECURSE GAME_SOURCES game/*.cpp)

# Create the executable
add_executable(two_half ${ENGINE_SOURCES} ${GAME_SOURCES})

# Include directories
# - engine/include gives us <TwoHalfD/engine.h>
# - game allows you to include your own headers easily
target_include_directories(two_half
    PRIVATE
        game
        ${CMAKE_SOURCE_DIR}/engine/include
)

# Link SFML libraries
target_link_libraries(two_half PRIVATE sfml-graphics sfml-window sfml-system sfml-audio)

# Enable compiler warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    target_compile_options(two_half PRIVATE -Wall -Wextra -Wpedantic)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(two_half PRIVATE -Wall -Wextra -Wpedantic)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(two_half PRIVATE /W4 /permissive-)
endif()

target_compile_definitions(two_half PRIVATE 
    ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
    ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# Optional: treat warnings as errors (uncomment to enable)
# target_compile_options(two_half PRIVATE -Werror)  # for Clang/GCC
# target_compile_options(two_half PRIVATE /WX)      # for MSVC
