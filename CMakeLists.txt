cmake_minimum_required(VERSION 3.10)
project(manganese)

set(CMAKE_CXX_STANDARD 17)
set(ROOT_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/manganese.cpp")

# Prevent in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

# Find all source files recursively from src directory only
file(GLOB_RECURSE SOURCES 
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)

# Filter out files with "_old" in their name
list(FILTER SOURCES EXCLUDE REGEX ".*_old.*")

# Add executable with all source files
add_executable(${PROJECT_NAME} ${SOURCES} ${ROOT_SOURCE})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Output binary to bin directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

