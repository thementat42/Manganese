# === Data Encapsulation ===

Manganese supports different data encapsulation methods. Data encapsulation methods allow functionality to be abstracted away from the user. Manganese uses slightly different terminology for its data encapsulation methods than other languages. The following sections describe the different data encapsulation methods in Manganese.

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

## === Blueprints ===

Blueprints are Manganese's version of classes. Like `bundle`s, they can contain variables; but unlike `bundle`s, they can also contain member functions. The general syntax for a blueprint is as follows:

```manganese
blueprint <blueprint name> {
    <data type> <variable1>;
    <data type> <variable2>;
    <data type> <variable3> = <value>;
    ...
    
    func constructor((optional parameters)) -> (<blueprint name>) {
        # code
    }

    func destructor() {
        # code
    }

    func <function name>((optional parameters)) -> (<return type>) {
        # code
    }
}
```

Members of a blueprint can be accessed using the dot operator (`.`), like with `bundle`
By default, members of a blueprint are `private` -- they cannot be accessed or modified outside the blueprint. This can be changed using the `public` keyword, which makes the member accessible and modifiable outside the blueprint. The `readonly` keyword can also be used to make a member accessible but not modifiable outside the blueprint.

### === Constructors and Destructors ===

`constructor` and `destructor` are special functions that are called when an object of the blueprint is created and destroyed, respectively. Like `main`, they are not reserved keywords, but will always be interpreted as the constructor or destructor of the blueprint. The return type of the constructor must be the blueprint name (and cannot be an error type). The destructor will return an error if any cleanup fails, and `0` if the cleanup succeeds. The destructor is not required, but is recommended for blueprints that allocate memory or other resources. The constructor is also not required, but is recommended for blueprints that require initialization. If a constructor is not provided, the default constructor will be used, which initializes all members to their default values. If a destructor is not provided, the default destructor will be used, which does nothing.

### === Blueprint Member Functions ===

Blueprint member functions are declared like regular functions and are called using the `.` operator. Member functions can access and modify the members of the blueprint. Functions which are not meant to modify the members of the blueprint can be marked as `const`. This is enforced at compile time.

```manganese
blueprint Point {
    int x;
    int y;

    func constructor(int x, int y) -> Point {
        this.x = x;
        this.y = y;
    }
    func destructor() {
        # code
    }

    const func distance(Point p) -> int {  # cannot modify the members of the blueprint
        return ((x - p.x) ** 2 + (y - p.y) ** 2) ** 0.5;
    }
}
```
