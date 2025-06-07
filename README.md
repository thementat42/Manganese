# Manganese

![Logo](/logo.svg "Manganese Logo")
Manganese is a statically typed programming language built on the LLVM framework. It's a project inspired by an autocomplete text parsing task for ESC190 at the University of Toronto. Manganese compiles to LLVM IR, which is then translated into machine code.

This project is licensed under the MIT License. See [LICENSE-MIT](LICENSE-MIT) for details. The LLVM project itself is licensed under the Apache License 2.0 with LLVM Exceptions ([LICENSE-APACHE](LICENSE-APACHE), also available on [LLVM's website](https://llvm.org/LICENSE.txt)).

## Why Manganese?

Manganese is mainly a fun project for learning about compilers and low-level programming. It's not meant for production use and lacks features like a full standard library. Please don't use this for any important projects.

### Dependencies

Manganese requires the following dependencies to be installed:

- [LLVM](https://llvm.org/) (version 16 or later)
- [CMake](https://cmake.org/) (version 3.10 or later)
- [Python](https://www.python.org/) (version 3.8 or later), if you want to use the `build.py` script

## Building

To build from source, first clone the repository:

```bash
git clone https://github.com/thementat42/Manganese.git
cd Manganese
```

Then, either use the Python script for a simple build:

```bash
python build.py
```

The Python script has different command line arguments to control the build process. Run `python build.py --help` to see the available options.
The python script requires `CMake` to be installed and available in the system's `PATH`.

If you don't have python, you can install it from [the official Python website](https://www.python.org/) or use CMake directly:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

This will create an executable called `manganese`. To compile Manganese code, use:

```bash
manganese <source file> -o <output file>
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

Then, move the `manganese-tests` executable to the root directory and run the tests:

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

## File Structure

This project is divided into several directories:

- [`.spec`](/.spec/): Contains the formal specification of the manganese language, including its EBNF grammar

- [`Docs`](/docs): Language documentation. The documentation represents a hypothetical version of Manganese -- right now, not everything in the docs is implemented, but will be at some point in the future.
  - [`Syntax`](/docs/syntax/): Documentation for the core language syntax.
  - [`Library`](/docs/library/): Documentation for the standard library

- [`Examples`](/examples): Sample Manganese programs

- [`Src`](/src): The compiler source code
  - [`Frontend`](/src/frontend): Lexer, parser, semantic analyzer
  - [`Middleend`](/src/middleend): Generates LLVM IR, runs LLVM's optimization passes
  - [`Backend`](/src/backend): LLVM backend to generate machine code
  - Each of these directories has an `include` directory which contains the header files for that phase.
  - [`global_macros`](/src/global_macros.h): Contains some macros which are useful everywhere.
- [`Tests`](/tests): Compiler Tests

- [`manganese.cpp`](/manganese.cpp): Entry point for the compiler
- [`manganese-tests.cpp`](/manganese-tests.cpp): Entry point for running tests

### Miscellaneous Files

The root directory has some other files as well:

- [`build.py`](/build.py): Python script to build the compiler
- [`.clang-format`](/.clang-format)
- [`CMakeLists.txt`](/CMakeLists.txt): CMake build script
- [`LICENSE-MIT`](/LICENSE-MIT) and [`LICENSE-APACHE`](/LICENSE-APACHE): License files
  - The project is dual-licensed under the MIT License. LLVM is licensed under the Apache License 2.0 with LLVM Exceptions.
- [`logo.svg`](/logo.svg): The Manganese logo
