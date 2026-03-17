Valid Operator Expressions

Primitive types:
    int
    float
    char
    bool

Primitive-ish:
    string
    array

Notes:
    int here is a shorthand for any integer type (int(8/16/32/64) and uint(8/16/32/64))
        uint specifically means an unsigned integer type
    float is a shorthand for both float32 and float 64

## Arithmetic Operators

+
    int + int -> int
    int + float -> float
    float + int -> float
    float + float -> float
    
    +int -> int
    +float -> float

-
    int - int -> int
    int - float -> float
    float - int -> float
    float - float -> float

    -int -> int
    -float -> float

*
    int * int -> int
    int * float -> float
    float * int -> float
    float * float -> float

/
    int / int -> float
    int / float -> float
    float / int -> float
    float / float -> float

//
    int // int -> int
    int // float -> int
    float // int -> int
    float // float -> int

%
    int % int -> int

++
    ++int -> int
    int++ -> int
--
    --int -> int
    int-- -> int
    --float -> float
    float-- -> float

## Comparison Operators
Here, __op__ stands for any of {<, <=, >, >=, ==, !=}
    int __op__ int -> bool
    int __op__ float -> bool
    float __op__ int -> bool
    float __op__ float -> bool

    char __op__ char -> bool (compares ASCII values)


## Boolean Operators
    bool && bool -> bool
    bool || bool -> bool
    !bool -> bool

    Note: in if statements and loops, conditions will be implicitly cast to bool

== Bitwise Operators ==
Here, __op__ stands for any of {&, |, ^, <<, >>}
    int __op__ int -> int

~
    ~int -> int


== Miscellaneous Operators ==
&
    &{any variable} -> int64 (the address of the variable) (width depends on the architecture)
*
    *{any pointer variable} -> {type} (the type the pointer is pointing to)