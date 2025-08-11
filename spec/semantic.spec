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
    int here refers to any integer type (int(8/16/32/64) and uint(8/16/32/64))
        In the actual semantic analysis, these are treated differently -- this is just a shorthand
    float refers to both float32 and float 64
        In the actual semantic analysis, these are treated differently -- this is just a shorthand
    Things between ??? are maybes

=== Arithmetic Operators ===

+
    int + int -> int
    int + float -> float
    float + int -> float
    float + float -> float
    string + string -> string
    string + char -> string
    char + string -> string
    array + array -> array
        arrays must be of the same type and shape
        output is a new array whose length is the sum of the operand lengths
    
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
    string * uint -> string
        negative integers are not supported
    array * uint -> array
        negative integers are not supported

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

^^
    int ^^ int -> int
    int ^^ float -> float
    float ^^ int -> float
    float ^^ float -> float

++
    ++int -> int
    int++ -> int
    ++float -> float
    float++ -> float
--
    --int -> int
    int-- -> int
    --float -> float
    float-- -> float

=== Comparison Operators ===
Here, __op__ stands for any of {<, <=, >, >=, ==, !=}
    int __op__ int -> bool
    int __op__ float -> bool
    float __op__ int -> bool
    float __op__ float -> bool

    char __op__ char -> bool (compares ASCII values)
    string __op__ string -> bool (compare character by character -- on ties, shorter = smaller)
    array __op__ array -> bool (compare element by element -- on ties, shorter = smaller)


=== Boolean Operators ===
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
    &{any variable} -> int64 (the address of the variable)
*
    *{any pointer variable} -> {type} (the type the pointer is pointing to)