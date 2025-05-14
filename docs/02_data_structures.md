# === Data Structures ===

Manganese has 5 different built-in data structures. They are:

<!-- TODO? Make each associated with a standard library module instead? -->

- [strings](#-strings-)
- [static arrays](#-arrays)
- [vectors (dynamic arrays)](#-vectors-)
- [hashmaps](#-hashmaps-)
- [hashsets](#-hashsets-)

This doc describes the core features of these data structures. Some additional functionality is provided through the [functional module](/docs/library/functional.md).

Manganese uses 0-based indexing for element access, and supports negative indices (with -1 being the last element, -2 the second last, etc.)

## === Strings ===

Strings are a sequence of characters, initialized as a string literal. The syntax for declaring a string is:

```manganese
str variable (= <string literal>);
```

Where `<string literal>` is a set of characters enclosed in double quotes. Strings are immutable in Manganese -- once created the characters making up the string cannot be changed without re-assigning the variable.
<!-- TODO? Is there a way to allow mutable-by-default string literals -->

When accessing an element from a string using `[]`, the output type is a `char`.

### === String Operators ===

Strings support some of the operators outlined in the [operators doc](/docs/00_operators.md):

| Operator | Effect |
| -------- | ------ |
| `+`      | concatenates two strings, returning a new string.|
| `*`      | when the right-hand side is an integer, returns a new string with that many copies of the original string (e.g. "h"*3 outputs "hhh")|
| `==`     | `true` if two strings have all the same characters, false otherwise |
|`!=`      | opposite of the `!=` operator |
| `[]`     | Returns the character at a given index |

### === String Methods ===

Strings support different methods. They are:

| Method | Effect | Return Type |
| ------ | ------ | ----------- |
| `.count(value)` | Returns the number of times a value appears in the string (`0` if it doesn't) | `int64` |
| `.is_alpha()` | `true` if the string contains only alphabetic characters, `false otherwise | `bool` |
| `.is_alphanum()` | `true` if the string contains only alphanumeric characters, `false otherwise | `bool` |
| `.is_digit()` | `true` if the string contains only numeric characters, `false otherwise | `bool` |
| `.is_hex()` | `true` if the string contains only hexadecimal digits (), `false otherwise | `bool` |
| `.length()` | The length of the string | `int64` |
<!-- TODO? Slice? -->

## === Arrays

Arrays are static, ordered collections of items of the same type. The syntax for declaring an array is:

```manganese
arr<type, size> variable (={value1, value2, ...});
```

Any values which are not provided will contain the default value for the `<type>`.
Manganese also supports multidimensional arrays, with the syntax `arr<arr<type, size>, size>`. Multidimensional arrays can be indexed by chaining the `[]` operator (as in `array[0][1]`).

The `const` keyword can be used in two places in an array declaration:

- `const arr<type, size>` means the array variable cannot be reassigned, but its elements can be modified
- `arr<const type, size>` means the array variable can be reassigned, but the elements cannot be modified

Using `const` in both places results in an array which cannot be reassigned and whose elements cannot be modified.

### === Array Operators ===

| Operator | Effect |
| -------- | ------ |
| `+`      | concatenates two arrays, returning a new array.|
| `*`      | when the right-hand side is an integer, returns a new array with that many copies of the elements of the original array (e.g. `{1,2}*3` outputs `{1,2,1,2,1,2}`)|
| `==`     | `true` if two arrays have all the same elements, false otherwise |
|`!=`      | opposite of the `!=` operator |
| `[]`     | Returns the item at a given index |
| `[] =`     | Updates the item at a given index (the types must match) |

### === Array Methods ===

| Method | Effect | Return Type |
| ------ | ------ | ----------- |
| `.count(value)` | Returns the number of times a value appears in the array (`0` if it doesn't) | `int64` |
| `.index(value)` | Returns the position of the first occurrence of a value in an array, or an error if it does not exist | `int64\|NotFoundError` |
| `.length()` | The length of the array | `int64` |

## === Vectors ===

Vectors are dynamically-sized, ordered collections of items of the same type. The syntax for declaring a vector is:

```manganese
vec<type> variable (= {value1, value2, ...});
```

An uninitialized vector will have a size of 0 and a capacity of 1. If it is initialized with values, the initial capacity will be equal to the number of elements which were initialized.

When a vector reaches its capacity and another element is added, its capacity will double.

Like arrays, vectors can be multidimensional and are indexed using `[]`.

The `const` keyword can be used in two places in a vector declaration:

- `const vec<type, size>` means the vector variable cannot be reassigned (nor can it have elements added/removed), but its elements can be modified
- `vec<const type, size>` means the vector variable can be reassigned, but the elements cannot be modified

Using `const` in both places results in a vector which cannot be reassigned and whose elements cannot be modified.

### === Vector Operators ===

| Operator | Effect |
| -------- | ------ |
| `+`      | concatenates two vectors, returning a new vector.|
| `*`      | when the right-hand side is an integer, returns a new vector with that many copies of the elements of the original vector (e.g. `{1,2}*3` outputs `{1,2,1,2,1,2}`)|
| `==`     | `true` if two vectors have all the same elements, false otherwise |
|`!=`      | opposite of the `!=` operator |
| `[]`     | Returns the item at a given index |
| `[] =`     | Updates the item at a given index (the types must match) |

### === Vector Methods ===

| Method | Effect | Return Type |
| ------ | ------ | ----------- |
| `.append(value)` | Adds a value to the end of the vector | - |
| `.capacity()`    | Returns the number of elements the vector could hold | `int64` |
| `.clear()` | Removes all the elements from the vector | - |
| `.count(value)` | Returns the number of times a value appears in the vector (0 if it does not) | `int64` |
| `.extend(container)` | Adds all the elements of a container (another vector, or an array, or a set, of the same type) to the end of the vector | - |
| `.index(value)` | Returns the position of the first occurrence of a value in the vector, or an error if it does not exist| `int64\|NotFoundError`|
| `.length()` | The length of the vector (how many elements are currently in it) | `int64` |
| `.insert(value, index)` | Inserts a value at a specified index (up to the size of the array), and shifts all subsequent elements to the right | `OutOfBoundsError` if the index is invalid |
| `.reserve(size)` | Allocates enough space to hold a certain number of elements. If the size passed in is smaller than the current capacity, nothing happens | - |

## === Hashmaps ===

Hashmaps are unordered, dynamic collections of key-value pairs. The syntax for declaring a hashmap is:

```manganese
map<key type, value type (, optional hash function)> variable (= {key1: value1, key2: value2, ...});
```

An uninitialized hashmap contains no elements. All primitive types have a built-in hash function; any provided hash function must take one argument (of the same type as the key type), and return an integer -- this is enforced by the compiler.

Hashmaps are indexed using `[]`, where the value inside the square brackets is of the same type as the keys. Elements can be added to/overridden using `[] =` with a key -- if the key already exists in the map, its value will be updated, otherwise, it will be added.

The const keyword can be used in two places in a map declaration:

- `const map<key type, value type>` means the map variable cannot be reassigned (nor can it have elements added/removed), but its elements can be modified
- `vec<key type, const value type>` means the vector variable can be reassigned, but the values cannot be modified.
Using const in both places results in a map which cannot be reassigned and whose values cannot be modified.

The keys of a hashmap are always constant, and can never be modified.

Hashmaps do not support any operators except for `[]`.

### === Hashmap Methods ===

| Method | Effect | Return Type |
| ------ | ------ | ----------- |
| `.clear()` | Removes all key-value pairs from the map | - |
| `.get(key (, optional default))` | Gets a value from the map. If a default value is provided, will return that if the key is not found, otherwise, a NotFoundError | `key type \| NotFoundError` |
| `.has(key)` | Returns `true` if a hashmap has a key, `false` otherwise | `bool` |
| `.remove(key)` | Removes a key-value pair from the map. Returns a NotFoundError if the key is not in the map | `NotFoundError` if the key is not in the map |

## === Hashsets ===

Hashsets are unordered, dynamic collections of unique elements. The syntax for declaring a hashset is:

```manganese
set<type (, optional hash function)> variable (= {value1, value2, ...});
```

Like hashmaps, an uninitialized hashset contains no elements. All primitive types have a built-in hash function; any provided hash function must take one argument (of the same type as the key type), and return an integer -- this is enforced by the compiler.

Elements of sets cannot be accessed by index and cannot be modified

### === Hashset Operators ===

| Operator | Effect |
| -------- | ------ |
| `+` | Returns a new set which is the union of the two sets |
| `-` | Returns a new set which is the difference of the two sets |
| `*` | Returns a new set which is the intersection of the two sets |
| `/` | Returns a new set which is the symmetric difference of the two sets |

### === Hashset Methods ===

| Method | Effect | Return Type |
| ------ | ------ | ----------- |
| `.append(value)` | Adds a value to the set. Does nothing if the value already exists | - |
| `.clear()` | Removes all the elements from the set | - |
| `.extend(container)` | Adds all the elements of a container (another set, or an array, or a vector, of the same type) to the set, ignoring duplicates | - |
| `.has(value)` | Returns `true` if a value is in the set, `false` otherwise | `bool` |
| `.remove(value)` | Removes a value from the set. Does nothing if the value already exists | - |
| `.length()` | Returns the number of elements in the set | `int64` |

`.append()`, `.extend()` and `.remove()`, have corresponding functions (`.append_or_fail()`, `.extend_or_fail()` and `.remove_or_fail()`) which behave the same, but will return an ElementExistsError if the value that was passed in (or any value in the container in the case of `.extend()`) is already in the set.

## === Casting Between Data Structures

Certain casts are allowed between data structures. Any cast not mentioned below is not allowed.

### Casting Strings

Strings can be cast to any [primitive type](/docs/01_Variables.md#primitive-types), except `char`s, as well as arrays and vectors:

- `str` to `int`: If the string contains only digits, it will be converted to an integer with those digits (e.g. `"12345"` -> `12345`). If there are any non-digit characters, an error will be thrown (e.g. `"123a"` cannot be cast to an `int`).
  - Strings beginning with `0x` will be parsed as hexadecimal numbers. In these strings the characters `A` to `F` (uppercase or lowercase) are allowed. (e.g. `"0x1A"` -> `26`)
  - Strings beginning with `0o` will be parsed as octal numbers. In these strings, only the digits 0 to 7 (inclusive) are allowed. (e.g. `"0o77"` -> `63`)
  - Strings beginning with `0b` will be parsed as binary numbers. In these strings only the digits `0` and `1` are allowed (e.g. `"0b1001"` -> `9`)

- `str` to `float`: If the string contains only digits (and at most 1 decimal point), it will be converted to a floating point number with those digits (e.g. `"51.372"` -> `51.372`). If there are any non-digit characters (or more than 1 decimal point), an error will be thrown (e.g. neither `"1.3.1"` nor `"13.a"` cannot be cast to a `float`).
- `str` to `char`: Not supported -- however, strings can be cast to _arrays_ of characters (see below).
- `str` to `bool`: The empty string (`""`) becomes `false`, anything else becomes `true` (even a string containing only spaces, like `" "`, or the string `"0"`).
- `str` to `arr<char>`: Each character in the string is copied into the array. The size of the array must be greater than or equal to the length of the string, and must be an array of `char`s.
- `str` to `vec<char>`: Each character in the string is copied into the vector. The size of the vector will equal the length of the string, and the capacity will be equal to the length of the string. The vector must be a vector of `char`s.

### Casting arrays

- `arr` to `vec`: The elements of the array are copied into a new vector. The new vector has the same size and capacity as the array. If the type of the array is not the same as the type of the vector, each element of the array is cast to the type of the vector, assuming such a cast is valid.
- `arr` to `set`: The elements of the array are copied into a new set, ignoring duplicates. If the type of the array is not the same as the type of the set, each element of the array is cast to the type of the set, assuming such a cast is valid.

### Casting vectors

- `vec` to `arr`: The elements of the vector are copied into a new array. The size of the array must be greater than or equal to the size of the vector. If the type of the vector is not the same as the type of the array, each element of the vector is cast to the type of the array, assuming such a cast is valid.
- `vec` to `set`: The elements of the vector are copied into a new set, ignoring duplicates. If the type of the vector is not the same as the type of the set, each element of the vector is cast to the type of the set, assuming such a cast is valid.

### Casting hashsets

- `set` to `arr`: The elements of the set are copied into a new array. The size of the array must be greater than or equal to the size of the set. If the type of the set is not the same as the type of the array, each element of the set is cast to the type of the array, assuming such a cast is valid.
- `set` to `vec`: The elements of the set are copied into a new vector. The size of the vector must be greater than or equal to the size of the set, as is its capacity. If the type of the set is not the same as the type of the vector, each element of the set is cast to the type of the vector, assuming such a cast is valid. No order is guaranteed for the elements.
