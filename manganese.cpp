#include "src/global_macros.h"

#undef DEBUG
#define DEBUG 0  // Use this for release builds

#include <iostream>

int main(int argc, char const *argv[]) {
    std::cout << "Hello, World!" << '\n';
    for (size_t i = 1; i < argc; i++) {
        std::cout << "Arg" << i << ": " << argv[i] << '\n';
    }

    return 0;
}
