# ==============================================================================
# CMakeLists.txt for the Manganese Project
#
# This CMake configuration builds the Manganese compiler and its optional test
# suite. It supports various build options for debugging and memory tracking,
# and enforces best practices for build directory structure and compiler flags.
#
# Main Features:
#  - Requires CMake 3.10+ and LLVM (via find_package).
#  - Requires C++20
#  - Prevents in-source builds to keep source tree clean.
#  - Options:
#      * BUILD_TESTS: Enable building the test suite (default: OFF).
#      * MEMORY_TRACKING: Enable memory allocation tracking (default: OFF).
#      * CONTINUOUS_MEMORY_TRACKING: Enable detailed memory tracking (default: OFF). Requires MEMORY_TRACKING.
#  - Automatically defines preprocessor macros for memory/debug options.
#  - Configures strict compiler warnings and debug flags for test and debug builds.
#  - Links against LLVM and (for tests) gcov for coverage.
#  - Outputs binaries to the build/bin directory.
#
# Usage:
#   mkdir build
#   cd build
#   cmake .. [-DBUILD_TESTS=ON] [-DCMAKE_BUILD_TYPE="Debug"|"Release"] [-DMEMORY_TRACKING=ON] [-DCONTINUOUS_MEMORY_TRACKING=ON]
#   cmake --build .
#
# Notes:
#  - When BUILD_TESTS is ON, the test suite will be built with the DEBUG macro enabled
    # regardless of whether DEBUG is ON or OFF.
#  - CONTINUOUS_MEMORY_TRACKING requires MEMORY_TRACKING to be enabled. If it is not, a warning will be issued.
#  - Compiler flags are set to be extra strict in debug/test builds.
#  - Release builds use more aggressive optimizations.
# ==============================================================================


cmake_minimum_required(VERSION 3.10)
project(manganese)

# ==============================================================================
# Build configuration and options
# ==============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(MAIN "${CMAKE_CURRENT_SOURCE_DIR}/manganese.cpp")
set(TESTS_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/manganese_tests.cpp")

# Prevent in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new build directory and run CMake from there.")
endif()

# Default to a release build
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build (Debug/Release)" FORCE)
endif()

# Define include directories
set(INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Build options
option(BUILD_TESTS "Build the test suite" OFF)
option(MEMORY_TRACKING "Enable total memory allocation tracking (does not account for deallocations)" OFF)
option(CONTINUOUS_MEMORY_TRACKING "Enable continuous memory usage tracking (detailed logging to file)" OFF)

# ==============================================================================
# Memory tracking configuration
# ==============================================================================

if(MEMORY_TRACKING)
    add_compile_definitions(MEMORY_TRACKING=1)
    message(WARNING "Memory tracking is enabled. This will track total memory allocations but may not be 100% accurate, and will vary by compiler")
else()
    add_compile_definitions(MEMORY_TRACKING=0)
endif()

if(CONTINUOUS_MEMORY_TRACKING)
    if(NOT MEMORY_TRACKING)
        message(WARNING "CONTINUOUS_MEMORY_TRACKING requires MEMORY_TRACKING to be enabled. Continuous memory tracking will have no effect unless MEMORY_TRACKING is also enabled.")
    endif()
    message(WARNING "CONTINUOUS_MEMORY_TRACKING may not be 100% accurate and can vary between compilers and platforms.")
    add_compile_definitions(CONTINUOUS_MEMORY_TRACKING=1)
else()
    add_compile_definitions(CONTINUOUS_MEMORY_TRACKING=0)
endif()

# ==============================================================================
# LLVM configuration
# ==============================================================================

find_package(LLVM REQUIRED CONFIG)

if(NOT LLVM_FOUND)
  message(FATAL_ERROR "LLVM not found. Please install LLVM and make sure it is in the system PATH.")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
list(APPEND INCLUDE_DIRS ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# ==============================================================================
# Source files configuration
# ==============================================================================

file(GLOB_RECURSE SRC 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

if(BUILD_TESTS)
    file(GLOB_RECURSE TEST_SOURCES
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c"
        )

    set(TEST_SRC ${SRC} ${TEST_SOURCES})

    add_executable(${PROJECT_NAME}_tests ${TEST_SRC} ${TESTS_MAIN})

    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING
            "The test suite relies on the DEBUG macro being enabled.\n"
            "DEBUG will be forcibly set to ON for test builds."
        )
        set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
    endif()

    # Force DEBUG macro for the test target regardless of global option
    target_compile_definitions(${PROJECT_NAME}_tests PRIVATE DEBUG=1)
    
    if(MSVC)
        target_compile_options(${PROJECT_NAME}_tests PRIVATE
            /Od
            /Zi
            /W4
            /permissive-
            /WX
            /EHsc
            /RTC1
            /RTCu
            /GS
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${PROJECT_NAME}_tests PRIVATE
            -O0 -g3
            -Wshadow
            -Wuninitialized
            -Wnull-dereference
            -Wall
            -Wextra
            -Wformat
            -Wformat=2
            -Wconversion
            -Wimplicit-fallthrough
            -Werror
            -fno-omit-frame-pointer
            -fno-inline
            -fcheck-new
            -DGLIBCXX_DEBUG
            # --coverage
            # -fprofile-arcs
            # -ftest-coverage
            # -lgcov
            --save-temps
        )
        # target_link_libraries(${PROJECT_NAME}_tests PRIVATE gcov)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${PROJECT_NAME}_tests PRIVATE
            -O0 -g3
            -Wshadow
            -Wuninitialized
            -Wnull-dereference
            -Wall
            -Wextra
            -Wformat
            -Wformat=2
            -Wconversion
            -Wimplicit-fallthrough
            -Werror
            -fno-omit-frame-pointer
            -fno-inline
            -fcheck-new
            -Wno-unused-private-field
            -Wno-missing-prototypes
        )
    else()
        message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. Using generic compiler flags.")
        target_compile_options(${PROJECT_NAME}_tests PRIVATE
            -O0 -g3
            -Wall
            -Wextra
            -Werror
        )
    endif()
    
    # Include directories
    target_include_directories(${PROJECT_NAME}_tests PRIVATE ${INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME}_tests ${llvm_libs})
    
    # Output binary to bin directory
    set_target_properties(${PROJECT_NAME}_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    
    message(STATUS "Configured to build test suite")
else()
    add_executable(${PROJECT_NAME} ${SRC} ${MAIN})
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)        
        if(MSVC)
            target_compile_options(${PROJECT_NAME} PRIVATE
                /Od
                /Zi
                /W4
                /permissive-
                /WX
                /EHsc
                /RTC1
                /RTCu
                /GS
                /analyze
            )
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${PROJECT_NAME} PRIVATE
                -O0 -g3
                -Wshadow
                -Wuninitialized
                -Wnull-dereference
                -Wall
                -Wextra
                -Wformat
                -Wformat=2
                -Wconversion
                -Wimplicit-fallthrough
                -Werror
                -fno-omit-frame-pointer
                -DGLIBCXX_DEBUG
            )
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${PROJECT_NAME} PRIVATE
                -O0 -g3
                -Wshadow
                -Wuninitialized
                -Wnull-dereference
                -Wall
                -Wextra
                -Wformat
                -Wformat=2
                -Wconversion
                -Wimplicit-fallthrough
                -Werror
                -fno-omit-frame-pointer
                -Wno-unused-private-field
                -Wno-missing-prototypes
            )
        else()
            message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. Using generic debug flags.")
            target_compile_options(${PROJECT_NAME} PRIVATE
                -O0 -g3
                -Wall
                -Wextra
                -Werror
            )
        endif()
        message(STATUS "Building compiler in DEBUG mode")
    else()
        target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=0)
        if(MSVC)
            target_compile_options(${PROJECT_NAME} PRIVATE
                /O2
                /Ob2
                /Ot
                /EHsc
                /Qspectre
            )
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${PROJECT_NAME} PRIVATE
                -O3
                -finline-functions
                -funroll-loops
                -flto
                -fomit-frame-pointer
                -fstack-protector-strong
                -march=native
                -g0
            )
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${PROJECT_NAME} PRIVATE
                -O3
                -finline-functions
                -funroll-loops
                -flto
                -fomit-frame-pointer
                -fstack-protector-strong
                -march=native
                -g0
            )
        else()
            message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. Using generic release flags.")
            target_compile_options(${PROJECT_NAME} PRIVATE
                -O3
                -DNDEBUG
            )
        endif()
        message(STATUS "Building compiler in RELEASE mode")
    endif()

    target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${llvm_libs})

    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    message(STATUS "Configured to build main compiler")
endif()
