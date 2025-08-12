# Manganese

![Language: C++](https://img.shields.io/badge/language-C%2B%2B-00599C.svg?logo=c%2B%2B&logoColor=white)
[![License: MIT](https://img.shields.io/badge/License-MIT-teal.svg)](https://opensource.org/licenses/MIT)
![CMake](https://img.shields.io/badge/build%20system-CMake-064F8C.svg?logo=cmake)

![Logo](/logo.svg "Manganese Logo")
Manganese is a statically typed programming language built on the LLVM framework. It's a project inspired by an autocomplete text parsing task for ESC190 at the University of Toronto. Manganese compiles to LLVM IR, which is then translated into machine code.

This project is licensed under the MIT License. See [LICENSE-MIT](LICENSE-MIT) for details. The LLVM project itself is licensed under the Apache License 2.0 with LLVM Exceptions ([LICENSE-APACHE](LICENSE-APACHE), also available on [LLVM's website](https://llvm.org/LICENSE.txt)).

## Why Manganese?

Manganese is mainly a fun project for learning about compilers and low-level programming. It's not meant for production use and lacks features like a full standard library.

On the off chance that you were considering using Manganese for something important, please don't.

## Dependencies

Manganese requires the following dependencies to be installed:

- [LLVM](https://llvm.org/) (version 16 or later)
- [CMake](https://cmake.org/) (version 3.10 or later)
- [Python](https://www.python.org/) (version 3.8 or later), if you want to use the `build.py` scriptDependencies

## Building

To build from source, first make sure you have the [dependencies](#-dependencies-) installed.

Then, clone this repository:

```bash
git clone https://github.com/thementat42/Manganese.git
cd Manganese
```

### The Python Build Script

If you have python installed, you can use the [`build.py`](/build.py) script to automatically invoke CMake.
NOTE: The Python script requires `CMake` to be installed and available in the system's `PATH`.

```bash
python build.py
```

The Python script will automatically run CMake and build the executable using the system-configures build system (via `cmake --build`).
It also automatically moves the executable to the root directory.
The Python script has different command line arguments to control the build process. Run `python build.py --help` to see the available options.

### Building manually with CMake

If you don't have python, you can install it from [the official Python website](https://www.python.org/) or use CMake directly:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

This will create an executable called `manganese` in the `build/bin` directory, which should be moved to the project root.
To compile Manganese code, use:

```bash
./manganese <source file> -o <output file>
```

If no output file is specified, the source file name will be used (e.g. `foo.mn` becomes `foo`).

## Testing Framework

The [tests](/tests) directory contains tests for the compiler up to IR generation. To run the tests, enable tests during the build:

Using the Python script:

```bash
python build.py --tests
```

Or, using CMake directly, run the following commands in the `build` directory:

```bash
cmake .. -DBUILD_TESTS=ON
cmake --build .
```

Then, move the `manganese-tests` executable to the root directory and run the tests (the python script will automatically move the executable):

```bash
./manganese_tests [options]
```

The tests executable takes the following command line arguments:

- `--help`: Print this help message
- `--lexer`: Tests the lexer (checking that the lexer produces the expected tokens)
- `--parser`: Tests the parser (checking that the parser produces the expected AST)
- `--semantic`: Tests the semantic analyzer (checking that a semantically valid program is accepted and an invalid one is rejected)
- `--codegen`: Tests the code generation phase (checking that the generated LLVM IR is correct)
- `--all`: Run all tests

Running it with no arguments prints a help message. Any other arguments will be ignored.

Note that the testing executable must be in the root directory since it relies on relative paths to access the source files used for testing.

## File Structure

This project is divided into several directories:

- [`.spec`](/.spec/): Contains the formal specification of the manganese language, including its EBNF grammar

- [`docs`](/docs): Language documentation. The documentation represents a hypothetical version of Manganese -- right now, not everything in the docs is implemented, but will be at some point in the future.
  - [`Syntax`](/docs/syntax/): Documentation for the core language syntax.
  - [`Library`](/docs/library/): Documentation for the standard library

- [`examples`](/examples): Sample Manganese programs

- [`include`](/include): Contains the header files for the compiler, defining the public interface of the compiler, split by phase
  - [`frontend`](/include/frontend): Header files for the frontend phase (lexer, parser, semantic analyzer)
  - [`middleend`](/include/middleend): Header files for the middleend phase (LLVM IR generation, optimization passes)
  - [`backend`](/include/backend): Header files for the backend phase (LLVM backend to generate machine code)
  - [`io`](/include/io): Header files for the I/O library, which handles things like logging and file I/O

- [`src`](/src): The implementation of the compiler, split by phase. This mirrors the structure of the [`include`](/include/) directory.
  - [`frontend`](/src/frontend): Lexer, parser, semantic analyzer
  - [`middleend`](/src/middleend): Generates LLVM IR, runs LLVM's optimization passes
  - [`backend`](/src/backend): LLVM backend to generate machine code
  - [`io`](/src/io): I/O library implementation, which handles things like logging and file I/O

- [`scripts`](/scripts/): Contains some Python scripts to help generate snippets of C++ code
  - [`gen_header.py`]: Python script to generate a header file from a .cpp file
- [`tests`](/tests): Compiler Tests
- [`manganese.cpp`](/manganese.cpp): Entry point for the compiler
- [`manganese-tests.cpp`](/manganese-tests.cpp): Entry point for running tests

### Miscellaneous Files

The root directory has some other files as well:

- [`build.py`](/build.py): Python script to build the compiler
- [`.clang-format`](/.clang-format)
- [`CMakeLists.txt`](/CMakeLists.txt): CMake build script
- [`LICENSE-MIT`](/LICENSE-MIT) and [`LICENSE-APACHE`](/LICENSE-APACHE): License files
  - This project is licensed under the MIT License.
  - LLVM is licensed under the Apache License 2.0 with LLVM Exceptions.
- [`logo.svg`](/logo.svg): The Manganese logo
