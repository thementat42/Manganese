# === Modules ===

Modules allow code to be split across multiple files. Modules can be imported to access the function and variables declared in the module.

To declare a module, place `module <module name>` at the top of a file. This must be the first line in a module declaration (except for comments).
To import a module, use the `import` keyword followed by the module name. Modules can be aliased using the `as` keyword

```manganese

# in my_module.mn
module my_module
let x : int = 3;
# other code

# in main.mn
import my_module
import really_long_module_name as mod

let z : int = my_module.x;  # z is now 3

```

If multiple files have the same module name, they will be collectively treated as one big module when imported.

If multiple modules are grouped within a folder/directory, that folder/directory will be treated as a wrapper for the modules within it. So in the following file structure:

```files
| -- my_module
|    | -- foo.mn
|    | -- bar.mn
```

the syntax for importing the `foo` module is `import my_module::foo`.

## === Privacy ===

- `public`: If a variable is `public`, it can be accessed and modified outside the module in which it is declared (its "parent module")
- `readonly`: If a variable is `readonly`, it can be accessed outside its parent module, but cannot be modified
- `private`: If a variable is `private`, it can only be accessed within its parent module. This is the default access level

Functions only support `public` and `private` access, not `readonly`

By default, all variables and functions are `private`.

`public`, `readonly` and `private` can also be used to create blocks by placing a colon (`:`) after the keyword. Every variable and function declared after it, until the next block, will be assigned that access level. This is syntactic sugar for putting the access modifier in every variable declaration. For example:

```manganese
module my_module
public:
let x : int = 3;
let y : int = 4;

readonly:
let z : int = 5;

private:
func foo(a : int, b : int) -> int {
    return a + b;
}
```

This is equivalent to:

```manganese
module my_module
let x : public int = 3;
let y : public int = 4;
let z : readonly int = 5;
private func foo(a : int, b : int) -> int {
    return a + b;
}
```

If a variable has an access modifier specified within a block, it will override the access modifier of the block for that variable. If the specified modifier is less restrictive than the block in which it is declared, the compiler will issue a warning. For example:

```manganese
module my_module
private:
let w : int = 1;
let x : int = 3;
let y : int = 7;
int : readonly z = 5;

public:
let a : int  = 3;
let b : int  = 4;
let c : private int = 5;
```

Here `z` will be `readonly`, and `c` will be `private`, despite being declared in a `private` and `public` block respectively. The compiler will issue a warning for `z` since `readonly` is less restrictive than `private`, but not for `c` since `private` is more restrictive than `public`.
