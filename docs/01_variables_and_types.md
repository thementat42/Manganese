# === Variables ===

Manganese is a statically typed language, so all variables must be declared with their type. Manganese is also strongly typed, so the compiler will minimize implicit type conversions. When assigning a variable to another variable, Manganese copies the value on the right into the variable on the left.

The general syntax for declaring a variable in Manganese is:

```manganese
let <variable name> [: [<visibility>] <type qualifier> <type>] [= <value>];
const <variable name> [: [<visibility>] <type qualifier> <type>] = <value>;
```

`let` declares a mutable variable, while `const` declares an immutable variable.

Where:

- [`visiblity`](#-type-qualifiers--visibility-) dictates how the variable can be accessed outside the module in which it is declared
- a [`type qualifier`](#-type-qualifiers--visibility-) provides extra information to the compiler on how the variable should be interpreted and/or stored. A type qualifier is optional.
- `<type>` is one of the [primitive types](#-primitive-types-), a built-in [data structure](/docs/02_data_structures.md) or a [user-defined type](/docs/06_data_encapsulation.md)
- `variable name` is an identifier to refer to the variable. Variable names can contain letters, underscores and numbers (but cannot start with numbers).
- `value` is an optional initial value for the variable.

Type declarations are required on variables without an initial value.
Variables with an initial value generally do not require a type declaration, as the compiler will infer which type the variable is.
Variables which are declared without a value are uninitialized. Using an uninitialized variable will cause a warning.

## === Type Qualifiers & Visibility ===

Visibility modifiers dictate how an object is accessible between modules:

1. `public`: Declares that a variable can be accessed and modified outside its parent [module](/docs/05_modules_and_scoping.md)
2. `readonly`: Declares that a variable can be accessed but not modified outside its parent [module](/docs/05_modules_and_scoping.md)
3. `private`: Declares that a variable can only be accessed within its parent [module](/docs/05_modules_and_scoping.md)

Type qualifiers provide extra information to the compiler about how variables should be stored and read. The type qualifiers in Manganese are:

1. `const`: Declares a variable as constant (i.e.) immutable. <br>
    Once assigned, a `const` variable cannot have its value changed. Variables marked as `const` must have a value assigned when they are declared
2. `ptr`: Declares a pointer to a variable of the specified type (a `ptr` variable holds the memory address of the variable it points to).

`const` always applies to the type to its right.
By default, all variables are private -- they cannot be accessed or modified outside their parent module.

## === Primitive Types ===

The primitive types in Manganese are:

|   Type   |          Description           |                       Size                         |
| -------- | ------------------------------ | -------------------------------------------------- |
| `int`    | A signed integer               | [32 bits](#-integer-and-floating-point-precision-) |
| `uint`   | An unsigned integer            | [32 bits](#-integer-and-floating-point-precision-) |
| `float`  | A floating-point value         | [32 bits](#-integer-and-floating-point-precision-) |
| `char`   | A single character             | 8 bits                                             |
| `bool`   | A boolean (`true` or `false`)  | 8 bits                                             |

Some example variable assignments in Manganese are:

```manganese
let a : int = 3;  # defaults to a 32-bit integer
let big_number : int64 = 123456789012345;
let small_number : uint8 = 250;
let b : int = a;  # the value of a is copied into b
b = 7;  # b is 7, a is still 3
let c : char = 'c';
```

Note that characters are declared using single quotes

## === Primitive-ish Types ===

Manganese also has two types that behave like primitive types:

|   Type   |          Description           |                       Size                         |
| -------- | ------------------------------ | -------------------------------------------------- |
| `string` | A sequence of characters       | Variable (8 bits * length)                         |
| `T[]`    | An array of elements of type T | Variable (size of element * length)                |

Strings are declared using double quotes.
Arrays are homogenous, meaning that all the elements must be of the same type.

```manganese
let hello: string = "Hi!"
let names: string[] = ["Alice", "Bob"]
```

### === Integer and Floating-point precision ===

Manganese supports different integer and floating-point widths, which can hold different ranges of values, using the `int<width>`, `uint<width>` and `float<width>` syntax, where `width` is 8, 16, 32 or 64 (8 and 16 only apply for `int` and `uint`). If no width is specified, all three default to 32-bit values.

The following table summarizes the ranges of values for the different integer types in Manganese:

|    Type    |       Minimum Value       |       Maximum Value        |
| ---------- | -------------------------- | -------------------------- |
| `int8`     | -128                       | 127                        |
| `int16`    | -32 768                    | 32 767                     |
| `int32`    | -2 147 483 648             | 2 147 483 647              |
| `int64`    | -9 223 372 036 854 775 808 | 9 223 372 036 854 775 807  |
| `uint8`    | 0                          | 255                        |
| `uint16`   | 0                          | 65 535                     |
| `uint32`   | 0                          | 4 294 967 295              |
| `uint64`   | 0                          | 18 446 744 073 709 551 615 |
| `float32`  | 1.401298464324817e-45      | 3.4028234663852886e+38     |
| `float64`  | 5e-324                     | 1.7976931348623157e308     |

> Note: In future, 128-bit ints and floats will be added

## === Type Casting ===

Manganese allows casting between all the primitive types using the `as` keyword The general syntax for a type cast is:

```manganese
let <variable name> : <new type> = <value> as <new type>;
```

Casts can also be done in-place (e.g., when passing a variable to a function).

These casts are not re-interpretations of the bits that make up a value -- a new value is created based on the original value. Type casts do not modify the original value.

The following table summarizes the casting rules for converting between different primitive types in Manganese:

| Output > <br> Input v | `int`                     | `float`                  | `char`                     | `bool`                     |  `string` |
|------------------------|---------------------------|---------------------------|----------------------------|----------------------------| ------- |
| `int`                 | -                         | Adds `.0`                | ASCII/UTF-8 character      |  `0` -> `false`, else `true`| String representation  |
| `float`               | Truncates decimal part    | -                         | Truncated ASCII/UTF-8 char | `0.0` -> `false`, else `true`| String representation |
| `char`                | ASCII/UTF-8 value         | ASCII/UTF-8 as float      | -                          | `'\0'` -> `false`, else `true` | Same, but as a string |
| `bool`                | `false` -> `0`, `true` -> `1` | `false` -> `0.0`, `true` -> `1.0` | `false` -> `'\0'`, `true` -> `'1'` | - | `true` -> "true", `false` -> "false" |
| `string`              | If all digits, that value as `int`| If all digits, that value as `float` | Not supported | `false` if empty, `true` otherwise | - |

Below is more detail on how each primitive type can be cast. Note that arrays cannot be cast.

### === Casting `int`s ===

This applies to both signed and unsigned `int`s:

- `int` to `float`: The value of the integer is unchanged, but has a `.0` at the end (e.g. `1` --> `1.0`)
- `int` to `char`: The resulting type is the character with that integer as its ASCII/UTF-8 value (e.g. `65` -> `'A'`)
- `int` to `bool`: `0` becomes `false`, any other value becomes `true`
- `int` to `string`: The integer becomes as if it were wrapped in quotes (e.g. `1` -> `"1"`)
- Casting an `int` of a smaller width to one of a larger width (e.g. `int8` to `int32`) doesn't change the value
- Casting an `int` of a larger width to one of a smaller width (e.g. `int32` to `int8`) will truncate the value (only the least significant bits are preserved).
- Casting a `uint` to an `int` of the same or larger width has no effect. If the width of the `int` is smaller, the value will wrap around the maximum value that `int` can represent
- Casting an `int` to a `uint` will keep a positive value, and wrap a negative value around.

### === Casting `float`s ===

- `float` to `int`: The decimal part of the value is truncated (e.g. `65.5 -> 65`). If the width of the `int` is smaller than that of the `float`, the value will wrap around. Negative floats will also wrap around when casting to a `uint`
- `float` to `char`: The truncated value is converted to an ASCII/UTF-8 character (e.g. `65.3` -> `'A'`)
- `float` to `bool`: `0.0` becomes `false`, anything else becomes `true`
- `float` to `string`: The integer becomes as if it were wrapped in quotes (e.g. `2.5` -> `"2.5"`)
- Casting a `float64` to a `float32` will preserve only the least significant bits.

### === Casting `char`s ===

- `char` to `int`: The resulting integer is the ASCII/UTF-8 encoding for that character (e.g., `'A'` -> `65`)
- `char` to `float`: The resulting integer is the ASCII/UTF-8 encoding for that character, as a `float` (e.g., `'A'` -> `65.0`)
- `char` to `bool`: A null terminator (`'\0'`) becomes `false`, anything else becomes `true` (even a space or the character `'0'`)
- `char` to `string`: The char becomes a string (e.g. `'a'` -> `"a"`)

### === Casting `bool`s ===

- `bool` to `int`: `false` becomes `0`, `true` becomes `1`
- `bool` to `float`: `false` becomes `0.0`, `true` becomes `1.0`
- `bool` to `char`: `false` becomes a null terminator (`'\0'`), `true` becomes `'1'`
- `bool` to `string`: `true` becomes `"true"`, `false` becomes `"false"`

### === Casting `string`s

- `string` to `int`: If all the characters are digits, the result is an integer with that value (e.g. `"1234"` -> `1234`)
  - If the string contains non-digit characters, the cast stops at the first non-digit character, with a warning
- `string` to `float`: Similar to casting to an `int`, but one `.` is allowed
- `string` to `char` : Not supported
- `string` to `bool`: `true` if the string has characters, `false` if it is empty

### === Automatic Casting ===

The Manganese compiler will only automatically cast values in the following scenarios:

1. Assigning an integer value to a `float` variable will automatically cast it to a float (e.g. in `float x = 1 + 3;`, `x` will be `4.0`)
2. In loops and conditionals (see [the control flow doc](/docs/03_control_flow.md)), values will automatically be cast to `bool`s
3. Comparisons between an `int` and a `float` will compare the floating-point values of both (e.g. `3 > 3.14` will actually compute `3.0 > 3.14`)

## === Pointers ===

Pointers store memory addresses -- dereferencing a pointer accesses the value at that memory address.

### === Using `const` in pointer declarations

In a pointer declaration, `const` can be used in two ways:

- `const <variable name> : ptr <type>`: creates a pointer which cannot be moved (i.e., cannot be changed to point to another memory address). The underlying value can still be modified. This can be read as "a constant named `<variable name>` which is a pointer to a `<type>`"
- `let <variable name> : ptr const <type>`: creates a pointer which cannot be used to modify the value it points to, but can be moved to point to another memory address. This can be read as "a variable named `<variable name>` which is a pointer to a constant `<type>`"

Combining these two (`const <variable name> : ptr const <type>`) creates a pointer variable which can neither be reassigned nor used to modify its underlying value.

For more information on Manganese's memory management system, see the [memory documentation](/docs/10_memory_management.md).
