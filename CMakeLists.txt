cmake_minimum_required(VERSION 3.10)
project(manganese)

set(CMAKE_CXX_STANDARD 17)
set(COMPILER_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/manganese.cpp")
set(TESTS_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/manganese_tests.cpp")

# Build options
option(BUILD_TESTS "Build the test suite" OFF)
option(DEBUG "Build with debug symbols and no optimization" OFF)

# Find LLVM package
find_package(LLVM REQUIRED CONFIG)

if(NOT LLVM_FOUND)
  message(FATAL_ERROR "LLVM not found. Please install LLVM and make sure it's in your path.")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Prevent in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new build directory and run CMake from there.")
endif()

# Common source files for tests and compiler build
file(GLOB_RECURSE SRC 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frontend/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/io/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/middleend/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backend/include
)

if(BUILD_TESTS)
    file(GLOB_RECURSE TEST_SOURCES
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c"
        )
    list(APPEND INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    set(TEST_SRC ${SRC} ${TEST_SOURCES})

    # Create test executable with tests main file
    add_executable(${PROJECT_NAME}_tests ${TEST_SRC} ${TESTS_MAIN})

    # Always include debug symbols and DEBUG definition for test builds
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
            /analyz
        )
    else()
        # be extra pedantic in debug test builds
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
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
            -fno-omit-frame-pointer
            -fno-inline
            -fcheck-new
            -DGLIBCXX_DEBUG
            --coverage
        )
        target_link_libraries(${PROJECT_NAME}_tests PRIVATE gcov)  # add gcov to make sure everything is tested
    endif()
    
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
    # Add executable with all source files
    add_executable(${PROJECT_NAME} ${SRC} ${COMPILER_MAIN})    # Configure compiler options based on DEBUG flag
    if(DEBUG)
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
        else()
            # be extra pedantic in debug builds
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
                -fsanitize=address
                -fsanitize=undefined
                -fsanitize=leak
                -fno-omit-frame-pointer
                -fno-inline
                -fcheck-new
                -DGLIBCXX_DEBUG
            )
        endif()
        message(STATUS "Building compiler in DEBUG mode")
    else()
        target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=0)
        # be more permissive in release builds
        if(MSVC)
            target_compile_options(${PROJECT_NAME} PRIVATE
                /O2
                /Ob2
                /Ot
                /EHsc
                /Qspectre
            )
        else()
            target_compile_options(${PROJECT_NAME} PRIVATE
                -O3
                -s
                -finline-functions
                -funroll-loops
                -flto
                -fomit-frame-pointer
                -fstack-protector-strong
                -march=native
                -mspeculative-load-hardening
                -mindirect-branch=thunk
                -mfunction-return=thunk
                -g0
            )
        endif()
        message(STATUS "Building compiler in RELEASE mode")
    endif()

    # Include directories
    target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

    # Output binary to bin directory
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    
    message(STATUS "Configured to build main compiler")
endif()

