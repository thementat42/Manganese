# === Control Flow ===

Control flow is used to modify the execution of a program from a linear progression through the file. Manganese supports has `if`, `switch` and `return` statements, `break` and `continue` instructions and `for`, `repeat`, `while` and `do/while` loops.

## === Conditionals ===

Conditionals allow for branched execution of statements. The general syntax for a conditional in Manganese is:

```manganese
if (boolean expression) {
    statement(s);
} elif (boolean expression) {
    statement(s);
} else {
    statement(s);
}
```

both `elif` and `else` blocks must have an associated `if` block. Manganese always requires braces to enclose the statements of a boolean expression, even if it is a single line.

### === Switch Statements ===

Switch statements are used to match a variable against a series of constant values. The general syntax for a switch statement is:

```manganese
switch (variable) {
    case value1:
        statement(s);
    case value2:
        statement(s);
    etc.
    default:
        statement(s);
}
```

Once a case has been met, the compiler will fall through all subsequent cases. If no case is met, the `default` block is executed. This can be prevented using the `break` statement, which will jump to the end of the `switch` block.

The `default` block is optional -- if no case is met and the `default` block is not present, the entire switch statement is a no-op.

## === Loops ===

Loops allow a piece of code to be executed repeatedly while some condition remains true.

Loops can be exited early using a `break` statement or have iterations skipped using `continue`.

### === For Loops and Repeat loops ===

`for` loops iterate over a range of values. The general structure of a `for` loop is:

```manganese
for (initialization; stop condition; step) {
    statement(s);
}
```

- The `initialization` portion is run once before the loop begins. It can contain multiple expressions, each separated by a comma.
- The `stop condition` is a single boolean expression which is checked before each iteration. Once it is `false`, the loop halts
- The `step` portion is run at the end of each iteration. It can also contain multiple expressions, each separated by a comma.

---

`repeat` loops are for when a block of code needs to be executed multiple times, but the current iteration number is not important. The structure of a repeat loop is:

```manganese
repeat (integer value){
    statement(s);
}
```

If the value in a repeat loop is a floating-point value, the number of iterations will be that number, rounded up to the nearest integer (e.g. `repeat (3.3)` will execute 4 times).

### === While and Do/While Loops ===

While loops will continue executing until some condition is `false`. The syntax for a `while` loops is:

```manganese
while (boolean expression){
    statement(s);
}
```

A `do`/`while` loop is almost identical to a `while` loop, but guarantees that the block will be executed once before the condition is checked. The syntax for a `do`/`while` loop is:

```manganese
do {
    statement(s)
} while (boolean expression);
```

## === Returns ===

`return` ends the execution of a function and returns to its caller. The syntax for a return statement is

``` manganese
return (value);
```

If no value is given, the function simply halts execution.

See the [functions doc](/docs/04_functions.md) for more.
