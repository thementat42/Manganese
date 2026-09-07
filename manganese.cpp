#include <cstdio>

// This is a placeholder

int main(int argc, const char* argv[]) {
    std::printf("Hello from Manganese!\n");
    for (int i = 1; i < argc; ++i) { std::printf("Arg %d : %s\n", i, argv[i]); }
    return 0;
}
