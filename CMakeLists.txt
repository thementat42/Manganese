cmake_minimum_required(VERSION 3.10)
project(manganese LANGUAGES C CXX)

# Configuration

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prevent in-source builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# Default to a release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Build options
option(BUILD_TESTS "Build the test suite" OFF)
option(MEMORY_TRACKING "Enable memory allocation tracking" OFF)
option(CONTINUOUS_MEMORY_TRACKING "Enable continuous memory tracking" OFF)

# Memory tracking configuration

if(MEMORY_TRACKING)
    add_compile_definitions(MEMORY_TRACKING=1)
    message(STATUS "Memory tracking is enabled. This will track total memory allocations but may not be 100% accurate, and will vary by compiler")
else()
    add_compile_definitions(MEMORY_TRACKING=0)
endif()

if(CONTINUOUS_MEMORY_TRACKING)
    if(NOT MEMORY_TRACKING)
        message(WARNING "CONTINUOUS_MEMORY_TRACKING requires MEMORY_TRACKING to be enabled. Continuous memory tracking will have no effect unless MEMORY_TRACKING is also enabled.")
    endif()
    message(STATUS "CONTINUOUS_MEMORY_TRACKING may not be 100% accurate and can vary between compilers and platforms.")
    add_compile_definitions(CONTINUOUS_MEMORY_TRACKING=1)
else()
    add_compile_definitions(CONTINUOUS_MEMORY_TRACKING=0)
endif()

# LLVM Configuration

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set up sources

file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)


set(MAIN_SRC manganese.cpp)
set(TEST_MAIN_SRC manganese_tests.cpp)

if(BUILD_TESTS)
    file(GLOB_RECURSE TEST_SOURCES
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c"
        )
    add_executable(manganese ${SOURCES} ${TEST_SOURCES} ${TEST_MAIN_SRC})
    message(STATUS "Configured to build test suite")
else()
    add_executable(manganese ${SOURCES} ${MAIN_SRC})
    message(STATUS "Configured to build main compiler")
endif()

target_include_directories(manganese
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(manganese PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# `#define` stuff

target_compile_definitions(manganese PRIVATE
    DEBUG=$<OR:$<CONFIG:Debug>,$<BOOL:${BUILD_TESTS}>>
)

if(CONTINUOUS_MEMORY_TRACKING AND NOT MEMORY_TRACKING)
    message(WARNING "CONTINUOUS_MEMORY_TRACKING requires MEMORY_TRACKING")
endif()

# Flags
function(enable_strict_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4 /WX /permissive- /EHsc
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Werror -Wshadow -Wformat=2 -Wuninitialized -Wnull-dereference
            -Wconversion -Wimplicit-fallthrough -Wno-unused-private-field -Wno-missing-prototypes
            )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Werror -Wshadow -Wformat=2 -Wuninitialized -Wnull-dereference
            -Wconversion -Wimplicit-fallthrough -Wno-unused-private-field -Wno-missing-prototypes
        )
    else()
            message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. Using generic warning flags.")
            target_compile_options(${target} PRIVATE
            -Wall -Wextra -Werror
        )
    endif()
endfunction()

function(enable_debug_flags target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /Od /Zi /RTC1 /GS /analyze
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE
            -O0 -g3 -fno-omit-frame-pointer -fno-inline -fno-inline-functions -fcheck-new --save-temps
            -DGLIBCXX_DEBUG
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${target} PRIVATE
            -O0 -g3 -fno-omit-frame-pointer  -fno-inline -fno-inline-functions
        )
    else()
            message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. Using generic debug flags.")
            target_compile_options(${target} PRIVATE
            -O0
        )
    endif()
endfunction()

function(enable_release_flags target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /O2 /Ob2 /Ot /Qspectre
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE
            -O2 -finline-functions -funroll-loops -fomit-frame-pointer
            -fstack-protector-strong -march=native -g0
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${target} PRIVATE
            -O2 -finline-functions -funroll-loops -fomit-frame-pointer
            -fstack-protector-strong -march=native -g0
        )
    else()
            message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. Using generic debug flags.")
            target_compile_options(${target} PRIVATE
            -O2 -DNDEBUG
        )
    endif()

    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")  # -flto seems to break on windows
    target_compile_options(${target} PRIVATE -flto)
    target_link_options(${target} PRIVATE -flto)
    endif()
endfunction()

enable_strict_warnings(manganese)
if(BUILD_TESTS)
    enable_debug_flags(manganese)
elseif(CMAKE_BUILD_TYPE STREQUAL Debug)
    enable_debug_flags(manganese)
else()
    enable_release_flags(manganese)
endif()

target_include_directories(manganese SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
target_compile_options(manganese PRIVATE ${LLVM_DEFINITIONS_LIST})

llvm_map_components_to_libnames(LLVM_LIBS
    Core Support IRReader ExecutionEngine Analysis
    BitWriter CodeGen
)

target_link_libraries(manganese PRIVATE ${LLVM_LIBS})
