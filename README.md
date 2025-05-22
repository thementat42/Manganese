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
git clone https://github.com/thementat42/Manganese.git
```

Then run:

```bash
cd build
cmake ..
make
```

This will create an executable called `manganese` in the `build` directory. To compile code, create a file ending in `.mn` and write some manganese code. Check out the [examples](/examples/) directory for some samples.

You can then run:

``` bash
manganese <source file> -o <output file>
```

If no output file name is specified, the source file name will be used (e.g., foo.mn will output an executable named foo)

## Testing Framework

The [tests](/tests) directory contains tests for the portions of the compiler up to IR generation. The tests take in short snippets of Manganese code (as a string) and check if the output is as expected.
The tests use a simple [testrunner class](/tests/testrunner.h) which tracks success and failure.

The tests can be run using the [tests-main.cpp](/tests-main.cpp) file in the root directory. The file uses a unity built to run the tests (because I was too lazy to setup a second CMake). To run the test suite, first compile the tests-main file:

```bash
g++ -o mntests tests-main.cpp
```

(or substitute `g++` with your compiler).
Then run the tests:

```bash
./mntests [options]
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

- [`Src`](/src): Contains the source code for the compiler. Per the general LLVM structure, it is divided into three phases
  - [`Frontend`](/src/frontend): Contains the lexer (which splits the source code into tokens), the parser (which builds an abstract syntax tree from the tokens), and the semantic analyzer (which checks the AST for semantic errors)
  - [`Middle`](/src/middle): Takes in the AST and generates LLVM IR code. It then hands execution off to the LLVM backend for things like optimization and code generation.
  - [`Backend`](/src/backend): Contains the LLVM backend, which takes in the LLVM IR code and generates machine code.
  - Each of these directories has an `include` directory which contains the header files for that phase.
  - [`global_macros`](/src/global_macros.h): Contains some macros which are useful everywhere.
- [`Tests`](/tests): Contains tests for the compiler.

## Features

This section gives a brief overview of how Manganese works. For a more thorough explanation and how different edge cases are handled, see the documentation in the [`Docs`](/docs) directory.

There are three sub-sections in the docs library, which give more in-depth explanations of the language's [syntax](/docs/syntax/), the [standard library](/docs/library/) and some [example programs](/examples) if you're already familiar with programming.

There are also very thorough docs in the [root of the docs directory](/docs/) which

First, some high level things:

- Execution always starts in the `main` function of the file that is being compiled, like in C, C++ and Rust.
- Single line comments are denoted by `#`, and multi-line comments start with `/*` and end with `*/`.

### Variables

Variables in Manganese are declared with the following syntax:

```manganese
type_qualifier(s) type variable name (= <value>);
```

For example:

```manganese
const int x = 5;
```

The value is optional, and if not provided, will be default initialized (see [here](/docs/syntax/variables.md#default-initialization-rules)). Default initialization can be bypassed by using the `noinit` type qualifier.

Manganese supports all the basic primitive types (`int`, `float`, `bool`, `char` and `null`).

Finally, Manganese supports pointers (the `ptr`) type.

### Data Structures

> TODO: Add

### Control Flow

Manganese has basic control flow via `if`, `elif` and `else` blocks.
It also has `for`, `while`, `do while` and `repeat` loops. (`repeat` are for when a block of code needs to be executed multiple times but the iteration number is not important).

Manganese also supports simple pattern matching via `switch`/`case` statements and more complex pattern matching via `match`/`case` statements.

### Functions

Functions in Manganese are declared as follows:

```manganese
func function_name(type1 arg1, type2 arg2, ...) -> return_type {
    // function body
}
```

For example:

```manganese
func add(int x, int y) -> int {
    return x + y;
}
```

Functions can be overloaded based on the number of arguments and their types (but not by return type).
Functions can have default parameters and variadic arguments.

### Containers

#### Bundles

Manganese has some object-oriented features as well. It has `bundle`s, which are like `struct`s in C. They contain some variables arranged next to each other in memory. Like in C, they do not have member functions. They are declared as follows:

```manganese
bundle bundle_name {
    type1 variable1;
    type2 variable2;
    ...
}
```

For example:

```manganese
bundle Point {
    int x;
    int y;
}
```
