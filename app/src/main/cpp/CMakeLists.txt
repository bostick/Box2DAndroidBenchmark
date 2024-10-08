
cmake_minimum_required(VERSION 3.22.1)

project("Box2DBenchmark")

include(FetchContent)

set(BOX2D_BENCHMARKS ON)
set(BOX2D_ENABLE_SIMD OFF)

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
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    # latest commit on main
    GIT_TAG 2dbb6818cb5e69bd3dcd20b7190cdba3844dd4e8
)
FetchContent_MakeAvailable(box2d)

add_subdirectory(${CMAKE_SOURCE_DIR}/deps/raylib)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/raymob)

include_directories(${ANDROID_NDK}/sources/android/native_app_glue/)

add_library(box2dbenchmark-lib SHARED
    main.c
    joint_grid.c
    large_pyramid.c
    many_pyramids.c
    smash.c
    tumbler.c
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

target_compile_definitions(box2dbenchmark-lib PRIVATE PLATFORM_ANDROID)

target_include_directories(box2dbenchmark-lib PRIVATE "${CMAKE_SOURCE_DIR}/deps/raylib")
target_include_directories(box2dbenchmark-lib PRIVATE "${CMAKE_SOURCE_DIR}/deps/raymob")

target_link_libraries(box2dbenchmark-lib raylib raymoblib box2d enkiTS)

set_target_properties(box2dbenchmark-lib
    PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)








