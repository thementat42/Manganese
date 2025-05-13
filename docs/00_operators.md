# === Operators ===

Operators are specific, non-alphanumeric characters which the compiler will interpret as an instruction to perform an action. These symbols cannot be used in any other context (such as variable names).

Manganese supports a variety of different operators, which can be grouped into different types.

## === Arithmetic Operators ===

| Operator |            Name              |     Example    |
| -------- | ---------------------------- | -------------- |
|    `+`   |   Addition/Unary Positive    |  `x+y` or `+x` |
|    `-`   |   Subtraction/Unary Negative | `x-y` or `-x`  |
|    `*`   |   Multiplication             |     `x*y`      |
|    `/`   |   Floating point division    |     `x/y`      |
|   `//`   |   Floor (integer) division   |    `x//y`      |
|    `%`   |   Modulus (Remainder)        |     `x%y`      |
|   `**`   |   Exponentiation             |    `x**y`      |
|   `++`   |   Increment                  | `++x` or `x++` |
|   `--`   |   Decrement                  | `--x` or `x--` |

All the binary operators support the immediate re-assignment syntax using `op=`.

Binary operators will typically return an integer if and only if both operands are an integer. Otherwise, the resulting output will be a floating point number.

The exception are the division operators:

- `/` will always return a float
- `//` and `%` will always return an int

Unary operators will preserve the type they act on. The increment and decrement operators can be used either as prefix or as postfix operators (e.g. `x++` vs `++x`). The postfix version increments the value of `x` after using it, whereas the prefix version uses the value after incrementing it.

## === Comparison Operators ===

| Operator |            Name              |    Example  |
| -------- | ---------------------------- | ----------- |
|    `<`   |     (Strictly) Less than     |   `x < y`   |
|   `<=`   |    Less than or equal to     |   `x <= y`  |
|    `>`   |    (Strictly) Greater than   |   `x > y`   |
|   `>=`   |    Greater than or equal to  |   `x >= y`  |
|   `==`   |          Equal to            |   `x == y`  |
|   `!=`   |         Not equal to         |   `x != y`  |

Manganese supports comparison operator chaining (e.g. `x < y < z`). Note that, due to hardware limitations, some comparisons between floating-point values may give unexpected results.

## === Boolean (Logical) Operators ===

| Operator |            Name              |     Example  |
| -------- | ---------------------------- | ------------ |
|   `&&`   |         Logical AND          |   `x && y`   |
|  `\|\|`  |         Logical OR           |   `x \|\| y` |
|   `!`    |         Logical NOT          |     `!x`     |

The logical AND and OR operators use short-circuiting for efficiency:

- In `x && y` if `x` is `false`, `y` is never evaluated, and the whole expression is treated as `false`
- In `x || y` if `x` is `true`, `y` is never evaluated and the whole expression is treated as `true`

## === Bitwise Operators ===

Bitwise operators work similarly to logical operators, but (as their name suggests), they operate on the individual bits that make up a value, rather than using the truthiness of the value as a whole.

| Operator |            Name              |    Example  |
| -------- | ---------------------------- | ----------- |
|   `&`    |        Bitwise AND           |   `x & y`   |
|   `\|`   |        Bitwise OR            |   `x \| y`  |
|   `^`    |        Bitwise XOR           |   `x ^ y`   |
|   `~`    |        Bitwise NOT           |   `x ~ y`   |
|  `<<`    |     Bitwise Left Shift       |   `x << y`  |
|  `>>`    |     Bitwise Right Shift      |   `x >> y`  |

Bitwise operators support the immediate re-assignment syntax (`op=`)

## === Brackets ===

|  Bracket |            Purpose        |     Example    |
| -------- | ------------------------- | -------------- |
| `()`     |  Function calls, grouping |     `foo()`    |
| `[]`     |       Element access      |     `foo[3]`   |
| `{}`     |  Scope/Block definitions  |  `{some code}` |
| `<>`     |        Generics           | `func<T> foo()`|

## === Miscellaneous Operators ===  

These operators don't fit in any other category

| Operator |       Name       |       Example       |
| -------- | ---------------- | ------------------- |
|    `?`   | Address of       |       `?x`          |
|    `@`   | Dereference      |      `@p_x`         |
|    `.`   | Member access    |       `x.y`         |
|   `::`   | Scope Resolution |     `module::y`     |
|    `=`   | Assignment       |      `x = y`        |
|   `->`   | Arrow            | `func foo() -> int` |
| `cast<>` | Type cast        | `cast<int> x`       |

Note that the address of operator (`?`) and the dereference operator (`@`) defer from the C-style `&` and `*` operators.

Manganese uses copy-based assignment. An expression like `x = y` copies the value of `y` into `x`
<!-- Todo? Move? (rust-like) -->

## === Non-operator symbols ===

The following aren't really operators, but are symbols interpreted in a specific way

|  Symbol |      Purpose         |           Example           |
| ------- | -------------------- | --------------------------- |
|   `#`   | Inline comments      | `x++;  # Ignore this value` |
| `/* */` | Multiline comments   |     `/* A docstring */`     |
|   `;`   | Statement terminator |       `statement;`          |

<!-- TODO? Nested block comments -->

## === Operator Precedence ===

In a multi-operator expression, operations are performed based on the following order (higher in the table means evaluated first). In the case of a tie, the operators are evaluated left-to-right.

Comments are always evaluated first (i.e., operators within comments are ignored.)

| Precedence | Operator | Description |
| ---------- | -------- | ----------- |
| 1 | `::` | Scope Resolution |
| 2 | `.` | Member access |
| 3 | `x++`<br> `x--`<br> `x()`<br> `x[]` | Increment (Postfix)<br> Decrement (Postfix) <br> Function Call <br> Indexing |
| 4 | `++x` <br> `--x` <br> `+x` <br> `-x`<br> `!`<br> `~`<br> `cast<>` <br> `@` <br> `?` | Increment (prefix) <br> Decrement (prefix) <br> Unary positive <br> Unary Negative <br> Logical NOT <br> Bitwise NOT <br> Type cast <br> Dereference <br> Address of <br> |
| 5 | `**` | Exponentiation |
| 6 | `*`<br> `/`<br> `//`<br> `%` | Multiplication<br> Floating-point division <br> Integer division <br> Modulus |
| 7 | `+`<br> `-` | Addition<br> Subtraction |
| 8 | `<<`<br> `>>` | Bitwise Left Shift<br> Bitwise Right Shift |
| 9 | `<`, `>`, `<=`, `>=` | Relational operators |
| 10 | `==`, `!=` | Equality operators |
| 11 | `&~` | Bitwise AND |
| 12 | `^~` | Bitwise XOR |
| 13 | `\|~` | Bitwise OR |
| 14 | `&` | Logical AND |
| 15 | `^` | Logical XOR |
| 16 | `\|` | Logical OR |
| 17 | `=`<br> `<op> =`| Assignment<br> Immediate Reassignment |
| 18 | `->` | Arrow |
