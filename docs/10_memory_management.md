# === Memory Management ===

At the moment, Manganese is not a memory-safe language. That said, it has some features to try and spot memory errors at compile time:

1. Whenever a function calls another function that returns a pointer type, the compiler will look for a call to `free` on the variable that the pointer points to, or a return statement that returns the pointer out of the function. If it finds neither, it will throw a warning.

    Note: This system is designed to be overly cautious, and may throw warnings even if the code does not leak memory. It is also not perfect -- just because a function does not trigger a warning does not guarantee that it is safe.

2. The `free` function marks the pointer as a null pointer -- this means that any attempts to access an already `free`d pointer will result in a compiler error about trying to read a null pointer.

> TODO: Work on a better memory management system that is more safe, but still flexible.
