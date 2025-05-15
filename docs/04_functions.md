# === Functions ===

Functions are blocks of code which can be reused throughout a program. Almost all code in Manganese is inside a function. The general syntax for a function is:

```manganese
func name(type1 parameter1, type2 parameter2, etc) -> return type {
    statement(s);

    return <value>;
}
```

If a function does not return a value, the return type can be omitted. Functions which do not return a value cannot be called in a variable assignment expression.

A parameter can be marked as a variadic argument by placing an ellipsis (`...`) before the variable name. Variadic arguments can be accessed like an array of the same type as the argument.

Functions can also assign parameters default values -- any parameters with default values must be placed after functions without them in the function's signature. If the parameter is not given a value, the default value will be used.

Functions can also be overloaded, provided they have unambiguously distinguishable types at compile time. Functions cannot be distinguished by return type alone.

An example function declaration might be:

```manganese
func foo(int x, int y, int z = 3, int... args) -> int {
    int64 sum = 0;
    for (int i = 0; i < args.length(); i++) {
        sum += args[i];
    }
    return x + y * z * sum;
}
```

<!-- ## === Generics ===

Manganese also supports generics -- parameters whose types are not known until the function is called. Functions can be marked as generic by using `func<T>` in their declaration (with multiple types being separated by commas). -->

## === Lambdas ===

Lambdas are anonymous functions, which can be declared inline for single-use cases. The syntax for declaring a lambda is:

```manganese
lambda (<input variables>) -> <return type>: <output expression>;
```

For example:

```manganese
lambda (int a, int b) -> int: a + b;
```

Unlike normal functions, lambdas must take at least one parameter and must always return a value.
