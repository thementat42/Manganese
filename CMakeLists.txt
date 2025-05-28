cmake_minimum_required(VERSION 3.10)
project(manganese)

set(CMAKE_CXX_STANDARD 17)
set(COMPILER_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/manganese.cpp")
set(TESTS_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/manganese-tests.cpp")

# Option to build the compiler tests instead of the main compiler
option(BUILD_TESTS "Build the test suite" OFF)

# Find LLVM package
find_package(LLVM REQUIRED CONFIG)

if(NOT LLVM_FOUND)
  message(FATAL_ERROR "LLVM not found. Please install LLVM and make sure it's in your path.")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Prevent in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

# Include directories common for both main compiler and tests
set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frontend/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/io/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/middleend/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backend/include
)
if(BUILD_TESTS)
  file(GLOB_RECURSE TEST_SOURCES
          "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
          "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c"
      )

    # Create test executable with tests main file
    add_executable(${PROJECT_NAME}_tests ${SOURCES} ${TESTS_MAIN})
    
    # Include directories
    target_include_directories(${PROJECT_NAME}_tests PRIVATE 
        ${INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    
    # Output binary to bin directory
    set_target_properties(${PROJECT_NAME}_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    
    message(STATUS "Configured to build test suite")
else()
    file(GLOB_RECURSE SOURCES 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    )
    # Add executable with all source files
    add_executable(${PROJECT_NAME} ${SOURCES} ${COMPILER_MAIN})

    # Include directories
    target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

    # Output binary to bin directory
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    
    message(STATUS "Configured to build main compiler")
endif()

