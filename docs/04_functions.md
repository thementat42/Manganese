# === Functions ===

Functions are blocks of code which can be reused throughout a program. Almost all code in Manganese is inside a function. The general syntax for a function is:

```manganese
func name(parameter1 : type1, parameter2 : type2, etc) -> return type {
    statement(s);

    return <value>;
}
```

If a function does not return a value, the return type can be omitted. Functions which do not return a value cannot be called in a variable assignment expression.

Functions can also assign parameters default values -- any parameters with default values must be placed after functions without them in the function's signature. If the parameter is not given a value, the default value will be used.

Functions can also be overloaded, provided they have unambiguously distinguishable types at compile time. Functions cannot be distinguished by return type alone.

An example function declaration might be:

```manganese
func foo(x: int, y: int, z: int = 3, ... args : int) -> int {
    let sum : in64 = 0;
    for (int i = 0; i < args.length(); i++) {
        sum += args[i];
    }
    return x + y * z * sum;
}
```

> TODO: (eventually) add generics

## === Lambdas ===

> TODO: Review this section (just have func without an identifier?)

Lambdas are anonymous functions, which can be declared inline for single-use cases. The syntax for declaring a lambda is:

```manganese
lambda (<input variables>) -> <return type>: <output expression>;
```

For example:

```manganese
lambda (int a, int b) -> int: a + b;
```

Unlike normal functions, lambdas must take at least one parameter and must always return a value.
