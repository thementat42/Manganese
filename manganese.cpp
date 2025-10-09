/**
 * @file manganese.cpp
 * @brief Entry point for the compiler.
 *
 * This file is responsible for calling the various components to compile source code
 */

#include <stdio.h>

// This is a placeholder

int main(int argc, const char* argv[]) {
    printf("Hello from Manganese!\n");
    for (int i = 1; i < argc; ++i) {printf("Arg %d : %s\n", i, argv[i]); }
    return 0;
}
