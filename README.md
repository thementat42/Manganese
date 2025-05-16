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

The value is optional, and if not provided, will be default initialized (see [here](/docs/syntax/variables.md#default-initialization-rules)). Default initialization can be bypassed by explicitly setting the value to `garbage` (a keyword in Manganese).

Manganese supports all the basic primitive types (`int`, `float`, `bool`, `char` and `null`) as well as strings (the `str` type).

Finally, Manganese supports pointers (the `ptr`) type. See [the memory section](#memory) for a quick overview and the [memory module](/docs/library/memory.md) for a more in-depth explanation.

### Data Structures

Manganese also supports arrays (both static and dynamic), hashmaps and sets. They are declared as follows:

```manganese
arr<type, size> variable_name (= [value1, value2, ...]);
vector<type> variable_name (= [value1, value2, ...]);
map<key_type, value_type> variable_name (= {key1: value1, key2: value2, ...});
set<type> variable_name (= {value1, value2, ...});
```

For example:

```manganese
arr<int, 5> x = [1, 2, 3, 4, 5];
vector<int> y = [1, 2, 3, 4, 5];
map<int, int> z = {1: 'a', 2: 'b', 3: 'c'};
set<int> w = {1, 2, 3, 4, 5};
```

Each of these also has default initialization rules, which can be found [here](/docs/syntax/data_structures.md).

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

#### Blueprints

Manganese also has `blueprint`s, which are like `class`es in C++. They contain member functions as well as member variables. They are declared as follows:

```manganese
blueprint blueprint_name {
    type1 variable1;
    type2 variable2;
    ...
    func function_name(type1 arg1, type2 arg2, ...) -> return_type {
        // function body
    }
}
```

For example:

```manganese
blueprint Point {
    int x;
    int y;
    func distance(Point other) -> float {
        return sqrt((x - other.x) * (x - other.x) + (y - other.y) * (y - other.y));
    }
}
```

Blueprints can also have constructors and destructors via the `constructor` and `destructor` methods inside the blueprint.

### Memory

Manganese implements a borrow checking system, similar to Rust. While direct references are not possible, pointers are. Pointers can be created using the `?` operator, and dereferenced using the `@` operator.
By default, all variables in Manganese are mutable; variables can be made immutable by using the `const` keyword. Pointers can be made immutable as well to prevent modifying the value they point to through the pointer. See [variables](/docs/syntax/variables.md) for more information. Similarly, function arguments can be made immutable to prevent modification of the argument inside the function.

### Error Handling
