# === Modules ===

Modules allow code to be split across multiple files. Modules can be imported to access the function and variables declared in the module.

To declare a module, place `module <module name>` at the top of a file. This must be the first line in a module declaration (except for comments).
To import a module, use the `import` keyword followed by the module name. Modules can be aliased using the `as` keyword

```manganese

# in my_module.mn
module my_module
int x = 3;
# other code

# in main.mn
import my_module
import really_long_module_name as mod

int z = my_module.x;  # z is now 3

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

By default, anything declared within a module is only accessible within that module (i.e., module elements are default private).
It is possible to change the access level of a module variable to one of three levels:

- `public`: If a variable is `public`, it can be accessed and modified outside the module in which it is declared (its "parent module")
- `readonly`: If a variable is `readonly`, it can be accessed outside its parent module, but cannot be modified

Functions only support `public` -- `public` functions can be called outside their parent module, while private functions cannot.

These access modifiers are placed along with other type qualifiers in variable declarations.

```manganese
public int x = 3;  # x can be accessed and modified outside this module
readonly int y = 7;  # y can be accessed outside this module, but not modified
int z = 9;  # z can neither be accessed nor modified outside this module
```
