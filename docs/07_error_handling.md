# === Error Handling ===

In Manganese, errors are treated as return types (similar to Zig).

```manganese
func name(parameters) -> success type|error type(s) {
    # code
}
```

When a function which might return an error is called, the compiler will enforce that each possible case is handled within a switch statement. For example:

```manganese
success type | error type(s) result = function(parameters);
switch (typeof(success)){
    case success type:
        # do something
        break;
    case error type:
        # error handling
        break;
    # etc.
}
```

The compiler will enforce that there is either a case statement for every error type, or that the switch statement has a `default` block (which can act as a catch-all).

Functions can also return an error type in the switch statement to bubble the error up to the caller.
