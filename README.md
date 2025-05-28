# Manganese

![Logo](/logo.svg "Manganese Logo")
Manganese is a programming language inspired by the autocomplete text parsing project for ESC190 at the University of Toronto and built on top of the LLVM framework. The language is a statically typed language with features inspired by various programming languages. The language is compiled to LLVM IR and then to machine code using the LLVM compiler infrastructure.

This project is licensed under the MIT License. This means you are free to use, modify and distribute the code as long as you include the original license and copyright notice. See the [LICENSE](LICENSE-MIT) file for more information.

This project is built on top of the LLVM compiler infrastructure. The LLVM project is licensed under the Apache License 2.0 with LLVM Exceptions. You can find the license [here](/LICENSE-APACHE) or [here](https://llvm.org/LICENSE.txt).

## Usage

Why should you use Manganese? There's a simple answer -- don't.
This is mainly meant as a fun project for me to learn more about compilers and low-level programming. Manganese lacks some useful features like a good standard library.

That said, if you just want to try it out, you're welcome to! I did try to make the syntax readable and clean, and I hope it at least emphasizes some good programming practices.

For now, you can [build it from source](#building)

### Building

This project uses CMake for its build system. To build the compiler from source, first clone this repo:

```bash
mkdir Manganese
cd Manganese
git clone https://github.com/thementat42/Manganese.git .
```

Then run:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

This will create an executable called `manganese` in the `build/bin` directory. To compile code, create a file ending in `.mn` and write some manganese code. Check out the [examples](/examples/) directory for some samples.

You can then run:

``` bash
manganese <source file> -o <output file>
```

If no output file name is specified, the source file name will be used (e.g., foo.mn will output an executable named foo)

## Testing Framework

The [tests](/tests) directory contains tests for the portions of the compiler up to IR generation. The tests take in short snippets of Manganese code (as a string) and check if the output is as expected.
The tests use a simple [testrunner class](/tests/testrunner.h) which tracks success and failure.

The tests can be built by adding the `-DBUILD_TESTS` flag when running CMake from the `build` directory:

```bash
cmake .. -DBUILD_TESTS=ON
cmake --build .
```

This will create an executable called `manganese-tests` in the `build/bin` directory. To run the tests, first move the `manganese-tests` executable to the root directory, then run it.

```bash
mv build/bin/manganese-tests .
./manganese-tests [--lexer | --parser | --semantic | --codegen | --all]
```

The tests executable takes the following command line arguments:

- `--lexer`: Run the lexer tests
- `--parser`: Run the parser tests
- `--semantic`: Run the semantic analyzer tests
- `--codegen`: Test the IR generation
- `--all`: Run all tests

Running it with no arguments prints a help message.

## File Structure

This project is divided into several directories:

- [`Docs`](/docs): Contains documentation for the language as markdown files. The documentation represents a hypothetical version of Manganese -- right now, not everything in the docs is implemented, but will be at some point in the future.
  - [`Syntax`](/docs/syntax/): Contains documentation for the actual syntax of the language
  - [`Library`](/docs/library/): Contains documentation for the standard library

- [`Examples`](/examples): Contains example programs written in Manganese

- [`Src`](/src): Contains the source code for the compiler.
  - [`core`](/src/core/): Contains some core elements (tokens, keyword and operator utils)
  - [`Frontend`](/src/frontend): Contains the lexer (which splits the source code into tokens), the parser (which builds an abstract syntax tree from the tokens), and the semantic analyzer (which checks the AST for semantic errors)
  - [`Middleend`](/src/middleend): Takes in the AST and generates LLVM IR code. It then hands execution off to the LLVM backend for things like optimization and code generation.
  - [`Backend`](/src/backend): Contains the LLVM backend, which takes in the LLVM IR code and generates machine code.
  - Each of these directories has an `include` directory which contains the header files for that phase.
  - [`global_macros`](/src/global_macros.h): Contains some macros which are useful everywhere.
- [`Tests`](/tests): Contains tests for the compiler.
- [`manganese.cpp`](/manganese.cpp): The main file which runs the compiler code
- [`tests-main.cpp`](/tests-main.cpp): The main file for running the tests
