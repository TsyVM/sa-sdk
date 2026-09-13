cmake_minimum_required(VERSION 3.16)
project(SASDK VERSION 0.1.0 LANGUAGES CXX
    DESCRIPTION "Production-grade GTA:SA modding SDK (sa10us build)")

# ---------------------------------------------------------------------------
# Standalone vs. subdirectory mode
# ---------------------------------------------------------------------------
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(SASDK_STANDALONE ON)
else()
    set(SASDK_STANDALONE OFF)
endif()

option(SASDK_BUILD_TESTS    "Build the host unit-test suite"          ${SASDK_STANDALONE})
option(SASDK_BUILD_EXAMPLES "Build the example programs"              ${SASDK_STANDALONE})
option(SASDK_WERROR         "Promote warnings to errors in SASDK code" OFF)

# ---------------------------------------------------------------------------
# Shared warning discipline (INTERFACE target -- not exported)
# ---------------------------------------------------------------------------
add_library(sasdk_warnings INTERFACE)
if(MSVC)
    target_compile_options(sasdk_warnings INTERFACE /W4 /permissive-)
    if(SASDK_WERROR)
        target_compile_options(sasdk_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(sasdk_warnings INTERFACE -Wall -Wextra -Wpedantic)
    if(SASDK_WERROR)
        target_compile_options(sasdk_warnings INTERFACE -Werror)
    endif()
endif()

# ---------------------------------------------------------------------------
# SASDK::data  --  compiled static library
#   Offline data utilities: error strings, IMG archive reader.
#   Does NOT depend on live game memory or x86 calling conventions.
#   Link this when writing offline tools (archive extractors, converters).
# ---------------------------------------------------------------------------
add_library(sasdk_data STATIC
    src/error.cpp
    src/img.cpp
    src/gxt.cpp
    src/col.cpp
    src/rw.cpp
    src/save.cpp
    src/fxp.cpp
    src/ide.cpp
    src/ipl.cpp
)
add_library(SASDK::data ALIAS sasdk_data)

target_include_directories(sasdk_data PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(sasdk_data PUBLIC cxx_std_20)
target_link_libraries(sasdk_data PRIVATE sasdk_warnings)

# ---------------------------------------------------------------------------
# SASDK::game  --  header-only runtime (in-process, 32-bit target only)
#   Struct database, hooking, invocation, typed globals, game bindings.
#   Designed to be compiled into a 32-bit .asi plugin.  Also builds
#   host-side (64-bit CI) for type-checking; calling-convention keywords
#   vanish on non-x86 (see include/sasdk/core/config.hpp).
# ---------------------------------------------------------------------------
add_library(sasdk_game INTERFACE)
add_library(SASDK::game ALIAS sasdk_game)

target_include_directories(sasdk_game INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(sasdk_game INTERFACE cxx_std_20)
# game layer uses error vocabulary from the static lib at link time
target_link_libraries(sasdk_game INTERFACE sasdk_data)
if(WIN32)
    target_link_libraries(sasdk_game INTERFACE kernel32)
endif()

# ---------------------------------------------------------------------------
# SASDK::sasdk  --  umbrella (consumers link this one target to get both)
# ---------------------------------------------------------------------------
add_library(sasdk INTERFACE)
add_library(SASDK::sasdk ALIAS sasdk)
target_link_libraries(sasdk INTERFACE sasdk_data sasdk_game)

# ---------------------------------------------------------------------------
# Host tests (CI)
#   Tests the portable logic: struct offsets, hook byte-encoding, address
#   math, global/view helpers.  Runs without a live game.
# ---------------------------------------------------------------------------
if(SASDK_BUILD_TESTS)
    enable_testing()
    add_executable(sasdk_test_core tests/test_core.cpp)
    target_link_libraries(sasdk_test_core PRIVATE sasdk sasdk_warnings)
    add_test(NAME core COMMAND sasdk_test_core)
endif()

# ---------------------------------------------------------------------------
# Examples
#   Compile on all platforms to prove the API is well-formed.
#   Runs on the real target only (game must be present).
# ---------------------------------------------------------------------------
if(SASDK_BUILD_EXAMPLES)
    add_executable(sasdk_example_vehicle_health examples/vehicle_health.cpp)
    target_link_libraries(sasdk_example_vehicle_health PRIVATE sasdk sasdk_warnings)
endif()
