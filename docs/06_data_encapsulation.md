# === Data Encapsulation ===

## === Bundles ===

Bundles in Manganese are similar to structs in C. They allow for simple data encapsulation, and are used to group related data together. Like structs in C, bundles do not have any methods or functions associated with them and do not support inheritance. They are simply a way to group related data together. Bundles are declared using the following syntax:

```manganese
bundle <bundle name> {
    <data type> <variable1>;
    <data type> <variable2>;
    <data type> <variable3> = <value>;
    ...
}
```

The default initialization rule for a bundle is to initialize each of its members using its default value. This is recursive, so if a bundle contains another bundle, the inner bundle will have all its fields initialized (this will repeat as needed). By default, the elements of a bundle are [`public`](/docs/05_modules_and_scoping.md#privacy) -- they can be accessed and modified outside the bundle. This can be changed using the `private` and `readonly` keywords. The `private` keyword makes the member inaccessible outside the bundle, while the `readonly` keyword makes variables accessible but not modifiable outside the bundle.

To access the members of a bundle, use the dot operator (`.`) as follows:

```manganese
<bundle name>.<member name>
```

For example:

```manganese
bundle Point {
    int x;
    int y;
}

bundle Circle {
    Point center;
    int radius;
}

Point p;  # default initialized to x = 0 and y = 0 (default value for int)
Circle c; # default initialized to center = (0, 0) (the center field is initialized as a default point) and radius = 0
p.x = 5; # set the x coordinate of the point to 5
c.center.y = 7; # set the y coordinate of the center of the circle to 7
c.center.y++; # increment the y coordinate of the center of the circle by 1
```

## === Enums ===

Enums in Manganese are similar to enums in C and C++. They allow for a set of named values to be defined. Enums are declared using the following syntax:

```manganese
enum <enum name> {
    <value1>;
    <value2>;
    <value3>;
    ...
}
```

Enums are useful for defining a set of named values instead of using magic numbers. Enums are also useful for defining a set of named values that can be used in a `switch` statement.

By default, the first value in an enum is assigned the value 0, and each subsequent value is assigned the next integer value. This can be overridden by explicitly assigning a value to a member of the enum. For example:

```manganese
enum Color {
    Red = 1;
    Green = 2;
    Blue = 3;
};
```

When a value is explicitly assigned, the next value in the enum will be assigned the next integer value. For example:

```manganese
enum Foo {
    Bar = 7;
    Baz; # Baz will be assigned the value 8
};
```

> TODO: Something more like a class
