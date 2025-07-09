

/**
 * @file manganese.cpp
 * @brief Entry point for the compiler.
 *
 * This file is responsible for calling the various components to compile source code
 */
#include <global_macros.h>

#include <iostream>

int main(int argc, const char* argv[]) {
    std::cout << "Hello, World!" << '\n';
    for (int i = 1; i < argc; i++) {
        std::cout << "Arg" << i << ": " << argv[i] << '\n';
    }
    return 0;
}
