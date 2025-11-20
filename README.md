# Manganese

![Language: C++](https://img.shields.io/badge/language-C%2B%2B-00599C.svg?logo=c%2B%2B&logoColor=white)
[![License: MIT](https://img.shields.io/badge/License-MIT-teal.svg)](https://opensource.org/licenses/MIT)
![CMake](https://img.shields.io/badge/build%20system-CMake-064F8C.svg?logo=cmake)
![Project Status: WIP](https://img.shields.io/badge/status-work_in_progress-orange)

<p align="center">
<img src="logo.svg" width="300" alt="Manganese Logo" />
</p>
Manganese is a statically-typed programming language built on top of the LLVM framework. It was inspired by the autocomplete text-parsing project
for ESC190 at the University of Toronto.

This project is licensed under the MIT License. See [LICENSE-MIT](LICENSE-MIT) for details. The LLVM project itself is licensed under the Apache License 2.0 with LLVM Exceptions ([LICENSE-APACHE](LICENSE-APACHE), also available on [LLVM's website](https://llvm.org/LICENSE.txt)).

## Quickstart

If you just want to get into this project:

```bash
git clone https://github.com/thementat42/Manganese.git
cd Manganese
python scripts/build.py
./manganese
```

> Note: Make sure you have the necessary [dependencies](#dependencies) installed

## Table of Contents

- [Why Manganese?](#why-manganese)
- [Dependencies](#dependencies)
- [Building the compiler](#building)
- [The testing framework](#testing-framework)
- [File Structure](#file-structure)

## Why Manganese?

Manganese is mainly an educational project for learning about compilers, programming language design and low-level programming.
> Manganese is currently a work-in-progress.

On the off chance that you were considering using Manganese for something important, please don't.

## Dependencies

Manganese requires the following dependencies to be installed:

- [LLVM](https://llvm.org/) (this project was made using version 20.1.7)
- [CMake](https://cmake.org/) (version 3.10 or later)
- [Python](https://www.python.org/) (version 3.8 or later), if you want to use the `build.py` script

## Building

To build from source, first make sure you have the [dependencies](#dependencies) installed.

Then, clone this repository:

```bash
git clone https://github.com/thementat42/Manganese.git
cd Manganese
```

### The Python Build Script

If you have Python installed, you can use the [`build.py`](/scripts/build.py) script to automatically invoke CMake.
> Note: The Python script requires `CMake` to be installed and available in the system's `PATH`.

```bash
python scripts/build.py
```

The Python script will automatically run CMake and build the executable using the system-configured build system (via `cmake --build`).
The Python script has different command line arguments to control the build process. Run `python scripts/build.py --help` to see the available options.

### Building with CMake

Building the project with CMake is relatively simple:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

This will create an executable called `manganese` in the `build/bin` directory.
<!-- To compile Manganese code, use:

```bash
./manganese <source file> -o <output file>
```

If no output file is specified, the source file name will be used (e.g. `foo.mn` becomes `foo`). -->

## Testing Framework

The [tests](/tests) directory contains tests for the compiler from lexical analysis through to LLVM IR generation. To run the tests, enable tests during the build:

Using the Python script:

```bash
python scripts/build.py --tests
```

Or, using CMake directly, run the following commands in the `build` directory:

```bash
cmake .. -DBUILD_TESTS=ON
cmake --build .
```

Then, move the `manganese` executable to the root directory and run the tests (the python script will automatically move the executable):

```bash
./manganese [options]
```

> Note: The testing executable _must_ be in the root directory since it relies on relative paths to access source files.

The tests executable takes the following command line arguments:

- `--help`: Print this help message
- `--lexer`: Tests the lexer (checking that the lexer produces the expected tokens)
- `--parser`: Tests the parser (checking that the parser produces the expected AST)
- `--semantic`: Tests the semantic analyzer (checking that a semantically valid program is accepted and an invalid one is rejected)
- `--codegen`: Tests the code generation phase (checking that the generated LLVM IR is correct)
- `--all`: Run all tests

Running it with no arguments prints a help message. Any other arguments will be ignored.

## File Structure

This project is divided into several directories:

- [`.spec`](/.spec/): Contains the formal(-ish) specification of the Manganese language, including its EBNF grammar

- [`docs`](/docs): Language documentation. The documentation represents a hypothetical version of Manganese; right now, not everything in the docs is implemented, but will be at some point in the future.

- [`examples`](/examples): Sample Manganese programs

- [`include`](/include): Contains the header files for the compiler, defining the public interface of the compiler, split by phase
  - [`frontend`](/include/frontend): Header files for the frontend phase (lexer, ast, parser, semantic analyzer)
  - [`middleend`](/include/middleend): Header files for the middleend phase (LLVM IR generation, optimization passes)
  - [`backend`](/include/backend): Header files for the backend phase (LLVM IR to machine code)
  - [`io`](/include/io): Various I/O utils (error logging and file reading)
  - [`utils`](/include/utils): miscellaneous utilities (string-to-number conversions, compiler configurations, memory tracking)

- [`src`](/src): The implementation of the compiler, split by phase. This mirrors the structure of the [`include`](/include/) directory.
  - [`frontend`](/src/frontend): Lexer, parser, semantic analyzer
  - [`middleend`](/src/middleend): Generates LLVM IR, runs LLVM's optimization passes
  - [`backend`](/src/backend): LLVM backend to generate machine code
  - [`io`](/src/io): Handles logging and file I/O
  - [`utils`](/src/utils): Utilities

- [`scripts`](/scripts/): Contains some utility Python scripts
  - [`build.py`](/scripts/build.py): Wraps CMake
  - [`lint.py`](/scripts/lint.py): Runs clang-tidy (requires clang-tidy to be in the PATH)
- [`tests`](/tests): The compiler test suite
- [`manganese.cpp`](/manganese.cpp): Entry point for the compiler
- [`manganese-tests.cpp`](/manganese-tests.cpp): Entry point for running tests

### Miscellaneous Files

The root directory has some other files as well:

- [`.clang-format`](/.clang-format)
- [`CMakeLists.txt`](/CMakeLists.txt): CMake build script
- [`LICENSE-MIT`](/LICENSE-MIT) and [`LICENSE-APACHE`](/LICENSE-APACHE): License files
  - This project is licensed under the MIT License.
  - LLVM is licensed under the Apache License 2.0 with LLVM Exceptions.
- [`logo.svg`](/logo.svg): The Manganese logo. This has no specific copyright.
