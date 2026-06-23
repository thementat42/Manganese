#include <stdio.h>

// This is a placeholder

int main(int argc, const char* argv[]) {
    printf("Hello from Manganese!\n");
    for (int i = 1; i < argc; ++i) {printf("Arg %d : %s\n", i, argv[i]); }
    return 0;
}
