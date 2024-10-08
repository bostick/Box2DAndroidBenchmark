# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.22.1)

# Set the project name based on the name given on the gradle.properties
project("Box2DBenchmark")

include(FetchContent)

set(BOX2D_BENCHMARKS ON)

SET(ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "Build enkiTS examples")

# Used in tests and samples
FetchContent_Declare(
    enkits
    GIT_REPOSITORY https://github.com/dougbinks/enkiTS.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(enkits)

FetchContent_Declare(
    box2d
    SOURCE_DIR /Users/brenton/development/github/box2d
)
FetchContent_MakeAvailable(box2d)

# Include raylib and raymob as a subdirectories
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/raylib)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/raymob)

# Add headers directory for android_native_app_glue.c
include_directories(${ANDROID_NDK}/sources/android/native_app_glue/)

# Create a shared library with game source files
add_library(box2dbenchmark-lib SHARED
    main.c
    joint_grid.c
    large_pyramid.c
    many_pyramids.c
    smash.c
    tumbler.c
)

# Define compiler macros for the library
target_compile_definitions(box2dbenchmark-lib PRIVATE PLATFORM_ANDROID)

# Apply flags depending on the build type
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_definitions(box2dbenchmark-lib PRIVATE _DEBUG DEBUG)
    set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-limit-debug-info")
    set(CMAKE_C_FLAGS_DEBUG "-g -fno-limit-debug-info")
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
endif()

# Include raylib and raymob header files
target_include_directories(box2dbenchmark-lib PRIVATE "${CMAKE_SOURCE_DIR}/deps/raylib")
target_include_directories(box2dbenchmark-lib PRIVATE "${CMAKE_SOURCE_DIR}/deps/raymob")

# Link required libraries to the native application
target_link_libraries(box2dbenchmark-lib raylib raymoblib box2d enkiTS)

set_target_properties(box2dbenchmark-lib
    PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)
