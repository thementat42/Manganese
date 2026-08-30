#include <array>
#include <core.hpp>
#include <cstddef>
#include <filesystem>
#include <frontend/parser.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#include "testrunner.hpp"

// NOTE: In the parser, any variable declaration without an explicit type is marked as 'auto'
// The semantic analysis phase is responsible for resolving the actual type
// So testing for a correct type resolution is outside the scope of these tests

namespace Manganese::tests {

constexpr static const char* logFileName = "logs/parser_tests.log";
static mnstl::chunk_allocator allocator;

// Helpers
namespace {
ast::Block getParserResults(const std::string& source, lexer::Mode mode = lexer::Mode::String) {
    parser::Parser parser(source, mode, allocator);
    parser::ParsedFile file = parser.parse();

    if (!file.moduleName.empty()) { std::cout << "module " << file.moduleName << "\n"; }
    if (!file.imports.empty()) {
        for (const auto& _import : file.imports) { std::cout << _import->toString() << "\n"; }
    }

    return std::move(file.program);
}

template <std::size_t N>
bool validateStatements(const ast::Block& block, const std::array<std::string, N>& expected, const char* testName) {
    std::ofstream logFile(logFileName, std::ios::app);
    if (!logFile) {
        std::cerr << "ERROR: Could not open log file for writing.\n";
    } else {
        logFile << "Test: " << testName << '\n';
    }
    std::cout << "Parsed " << testName << " AST:" << '\n';
    for (const auto& stmt : block) {
        const std::string stmtStr = stmt->toString(0);
        std::cout << stmtStr << '\n';
        if (logFile) {
            logFile << "String representation: " << stmtStr << '\n';
            logFile << "Dumping statement:\n";
            stmt->dump(logFile);
            logFile << "---------------------\n";
        }
    }
    if (logFile) { logFile.close(); }

    if (block.size() != N) {
        std::cerr << "ERROR: Expected " << N << " statements, got " << block.size() << " in test: " << testName << '\n';
        return false;
    }

    for (std::size_t i = 0; i < N; ++i) {
        std::string actual = block[i]->toString(0);
        if (actual != expected[i]) {
            std::cerr << "ERROR: Statement " << (i + 1) << " does not match expected in test: " << testName << '\n';
            std::cerr << "Expected: " << "\n" << expected[i] << '\n';
            std::cerr << "Actual:   " << "\n" << actual << '\n';
            return false;
        }
    }

    return true;
}

bool validateStatement(const ast::Block& block, const std::string& expected, const std::string& testName) {
    std::ofstream logFile(logFileName, std::ios::app);
    if (!logFile) {
        std::cerr << "ERROR: Could not open log file for writing.\n";
    } else {
        logFile << "Test: " << testName << '\n';
    }
    std::cout << "Parsed " << testName << " AST:" << '\n';
    for (const auto& stmt : block) {
        std::string stmtStr = stmt->toString(0);
        std::cout << stmtStr << '\n';
        if (logFile) {
            logFile << "String representation: " << stmtStr << '\n';
            logFile << "Dumping statement:\n";
            stmt->dump(logFile);
            logFile << "---------------------\n";
        }
    }

    if (block.size() != 1) {
        std::cerr << "ERROR: Expected 1 statement, got " << block.size() << " in test: " << testName << '\n';
        return false;
    }

    std::string actual = block[0]->toString(0);
    if (actual != expected) {
        std::cerr << "ERROR: Statement does not match expected in test: " << testName << '\n';
        std::cerr << "Expected: " << "\n" << expected << '\n';
        std::cerr << "Actual:   " << "\n" << actual << '\n';
        return false;
    }

    return true;
}
}  // namespace

static bool testArithmeticOperatorsAndCasting() {
    const std::string expression = "8 - 4 + 6 * 2 // 5 % 3 * 2 * 2 / 7 as float32;";
    std::string expected = "(((8 - 4) + ((((((6 * 2) // 5) % 3) * 2) * 2) / 7)) as float32);";

    return validateStatement(getParserResults(expression), expected, "Arithmetic Operators and Casting");
}

static bool testVariableDeclaration() {
    const std::string expression = "let mut foo = 45.5a;"
                                   "let mut bar = foo * 10;"
                                   "let baz : public uint32 = foo + 10 * 2 * bar + foo % 7 + foo*2;"
                                   "let boolean = true;";

    const std::array<std::string, 4> expected
        = {"(let mut foo: private auto = 45.5);", "(let mut bar: private auto = (foo * 10));",
           "(let baz: public uint32 = (((foo + ((10 * 2) * bar)) + (foo % 7)) + (foo * 2)));",
           "(let boolean: private auto = true);"};

    return validateStatements(getParserResults(expression), expected, "Variable Declaration");
}

static bool testAssignmentExpressions() {
    const std::string expression = "a = 5;\n"
                                   "b += 3;\n"
                                   "c -= 2 * b;\n"
                                   "d = -(c + 3);\n"
                                   "e *= f + 1;\n"
                                   "g /= h - -2;\n"
                                   "i %= 4;\n"
                                   "k //= 3;"
                                   "l = (3 + 4) * 2 - (1 + 1) * 5;"
                                   "a &= b;\n"
                                   "c |= d;\n"
                                   "e ^= f;\n"
                                   "g <<= 2;\n"
                                   "h >>= 3;\n"
                                   "i &= j | k;\n"
                                   "m |= n & p;\n"
                                   "x ^= ~y;\n";

    const std::array<std::string, 17> expected
        = {"(a = 5);",          "(b += 3);",       "(c -= (2 * b));",
           "(d = (-(c + 3)));", "(e *= (f + 1));", "(g /= (h - (-2)));",
           "(i %= 4);",         "(k //= 3);",      "(l = (((3 + 4) * 2) - ((1 + 1) * 5)));",
           "(a &= b);",         "(c |= d);",       "(e ^= f);",
           "(g <<= 2);",        "(h >>= 3);",      "(i &= (j | k));",
           "(m |= (n & p));",   "(x ^= (~y));"};

    return validateStatements(getParserResults(expression), expected, "Assignment Expressions");
}

static bool testPrefixOperators() {
    const std::string expression = "++x;\n"
                                   "--y;\n"
                                   "-z;\n"
                                   "+a;\n"
                                   "!b;\n"
                                   "-(d + 3);"
                                   "++c * 2;\n";

    const std::array<std::string, 7> expected
        = {"(++x);", "(--y);", "(-z);", "(+a);", "(!b);", "(-(d + 3));", "((++c) * 2);"};

    return validateStatements(getParserResults(expression), expected, "Prefix Operators");
}

static bool testParenthesizedExpressions() {
    const std::string expression = "(2 + 3) * 4;\n"
                                   "2 * (3 + 4);\n"
                                   "((5 + 2) * (8 - 3)) / 2;\n"
                                   "1 + (2 * (3 + 1));\n"
                                   "((2 + 3) * 4) - (6 / (1 + 1));";

    const std::array<std::string, 5> expected = {"((2 + 3) * 4);", "(2 * (3 + 4));", "(((5 + 2) * (8 - 3)) / 2);",
                                                 "(1 + (2 * (3 + 1)));", "(((2 + 3) * 4) - (6 / (1 + 1)));"};

    return validateStatements(getParserResults(expression), expected, "Parenthesized Expressions");
}

static bool testPointerOperators() {
    const std::string expression = "&variable;\n"
                                   "*pointer;\n"
                                   "**doublePointer;\n"
                                   "&(x + y);\n"
                                   "*p + 5;\n";

    const std::array<std::string, 5> expected
        = {"(&variable);", "(*pointer);", "(*(*doublePointer));", "(&(x + y));", "((*p) + 5);"};

    return validateStatements(getParserResults(expression), expected, "Pointer Operators");
}

static bool testTypedVariableDeclaration() {
    const std::string expression = "let mut x: int32 = 42;\n"
                                   "let y: public float64 = 3.14159;\n"
                                   "let mut z: char = 'A';\n"
                                   "let mut numbers: int32[3*2];\n"
                                   "let matrix: public float32[][] = [[1.0, 2.7], [3.0, 4.2]];\n";

    const std::array<std::string, 5> expected = {
        "(let mut x: private int32 = 42);", "(let y: public float64 = 3.14159);", "(let mut z: private char = 'A');",
        "(let mut numbers: private int32[(3 * 2)]);", "(let matrix: public float32[][] = [[1.0, 2.7], [3.0, 4.2]]);"};

    return validateStatements(getParserResults(expression), expected, "Typed Variable Declarations");
}

static bool testPostfixOperators() {
    const std::string expression = "x++;\n"
                                   "y--;\n"
                                   "(a + b)++;\n"
                                   "arr[i]--;\n"
                                   "++x--;\n"
                                   "x++ + y--;\n";

    const std::array<std::string, 6> expected
        = {"(x++);", "(y--);", "((a + b)++);", "(arr[i]--);", "(++(x--));", "((x++) + (y--));"};

    return validateStatements(getParserResults(expression), expected, "Postfix Operators");
}

static bool testBitwiseOperators() {
    const std::string expression = "a & b;\n"
                                   "c | d;\n"
                                   "e ^ f;\n"
                                   "~g;\n"
                                   "h << 2;\n"
                                   "i >> 3;\n"
                                   "(a & b) | (c ^ d);\n"
                                   "a & (b | c);\n"
                                   "~(a & b) | c;\n"
                                   "a & b & c | d ^ e;\n";

    const std::array<std::string, 10> expected = {"(a & b);",
                                                  "(c | d);",
                                                  "(e ^ f);",
                                                  "(~g);",
                                                  "(h << 2);",
                                                  "(i >> 3);",
                                                  "((a & b) | (c ^ d));",
                                                  "(a & (b | c));",
                                                  "((~(a & b)) | c);",
                                                  "(((a & b) & c) | (d ^ e));"};

    return validateStatements(getParserResults(expression), expected, "Bitwise Operators");
}

static bool testAggregateDeclarationAndInstantiation() {
    const std::string expression = "public aggregate Point {\n"
                                   "    x: uint128;\n"
                                   "    y: int128;\n"
                                   "    some_field: float64;\n"
                                   "}\n"
                                   "aggregate Rectangle {\n"
                                   "    topLeft: Point;\n"
                                   "    bottomRight: Point;\n"
                                   "    colour: uint32;\n"
                                   "}\n"
                                   "let mut p1 = Point{x = 10, y = 20};\n"
                                   "let mut p2: Point = Point{x = 30, y = 40};\n"
                                   "let rect = Rectangle{\n"
                                   "    topLeft = Point{x = 0, y = 0},\n"
                                   "    bottomRight = p2,\n"
                                   "    colour = 0xFF0000\n"
                                   "};\n";

    const std::array<std::string, 5> expected = {
        R"(public aggregate Point {
    x: uint128;
    y: int128;
    some_field: float64;
})",
        R"(private aggregate Rectangle {
    topLeft: Point;
    bottomRight: Point;
    colour: uint32;
})",
        "(let mut p1: private auto = Point {x = 10, y = 20});", "(let mut p2: private Point = Point {x = 30, y = 40});",
        "(let rect: private auto = Rectangle {topLeft = Point {x = 0, y = 0}, bottomRight = p2, colour = 16711680});"};

    return validateStatements(getParserResults(expression), expected, "Aggregate Declaration and Instantiation");
}

static bool testFunctionDeclarationAndCall() {
    const std::string expression = "public func add(a: int32, b: int32) -> int32 {\n"
                                   "    return a + b;\n"
                                   "}\n"
                                   "func greet(name: string) {\n"
                                   "    print(\"Hello, \" + name);\n"
                                   "}\n"
                                   "func calculate(x: float64, y: mut float64) -> float64 {\n"
                                   "    let result = x * y;\n"
                                   "    return result;\n"
                                   "}\n"
                                   "func increment(value: int32, amount: int32 = 1) -> int32 {\n"
                                   "    return value + amount;\n"
                                   "}\n"
                                   "func sum(first: int32, numbers...: int32) -> int32 {\n"
                                   "    return first;\n"
                                   "}\n"
                                   "let sum = add(5u32, 3i16);\n"
                                   "greet(\"World\");\n"
                                   "let product = calculate(2.5f64, 3.01);\n"
                                   "let incremented = increment(10);\n"
                                   "let total = sum(1, 2, 3, 4);\n";

    const std::array<std::string, 10> expected = {
        R"(public func add(a: int32, b: int32) -> int32 {
    return (a + b);
})",
        R"(private func greet(name: string) {
    print(("Hello, " + name));
})",
        R"(private func calculate(x: float64, y: mut float64) -> float64 {
    (let result: private auto = (x * y));
    return result;
})",
        R"(private func increment(value: int32, amount: int32 = 1) -> int32 {
    return (value + amount);
})",
        R"(private func sum(first: int32, numbers...: int32) -> int32 {
    return first;
})",
        "(let sum: private auto = add(5, 3));",
        "greet(\"World\");",
        "(let product: private auto = calculate(2.5, 3.01));",
        "(let incremented: private auto = increment(10));",
        "(let total: private auto = sum(1, 2, 3, 4));"};

    return validateStatements(getParserResults(expression), expected, "Function Declaration and Call");
}

static bool testLoops() {
    const std::string expression = "let i = 0;"
                                   "do {++i; print(i); } while (i < 5);"
                                   "let j: int32 = 10;"
                                   "while (true) {"
                                   "    if (j == 5) {continue;}"
                                   "    print(j--);"
                                   "    if (j <= 0) { break; }"
                                   "}"
                                   "for(let k = 0; k < 10; ++k) {print(k);}"
                                   "for (;;) {print(3);}"
                                   "for (i = 10;;) {print(3);}"
                                   "for (;i<4;) {print(3);}"
                                   "for (;;++i) {print(3);}";

    const std::array<std::string, 9> expected = {"(let i: private auto = 0);",
                                                 R"(do {
    (++i);
    print(i);
} while ((i < 5));)",
                                                 "(let j: private int32 = 10);",
                                                 R"(while (true) {
    if ((j == 5)) {
        continue;
    }
    print((j--));
    if ((j <= 0)) {
        break;
    }
})",
                                                 R"(for ((let k: private auto = 0); (k < 10); (++k)) {
    print(k);
})",
                                                 R"(for (;;) {
    print(3);
})",
                                                 R"(for ((i = 10); ;) {
    print(3);
})",
                                                 R"(for (;(i < 4); ) {
    print(3);
})",
                                                 R"(for (;;(++i)) {
    print(3);
})"};

    return validateStatements(getParserResults(expression), expected, "Loops");
}

static bool testIfElseStatements() {
    const std::string expression = "if (a < b) {\n"
                                   "    let result = a + b;\n"
                                   "    print(result);\n"
                                   "} elif (a > b) {\n"
                                   "    let result = a - b;\n"
                                   "    print(result);\n"
                                   "} else {\n"
                                   "    print(\"Equal\");\n"
                                   "}";

    std::string expected = R"(if ((a < b)) {
    (let result: private auto = (a + b));
    print(result);
} elif ((a > b)) {
    (let result: private auto = (a - b));
    print(result);
} else {
    print("Equal");
})";

    return validateStatement(getParserResults(expression), expected, "If/Else If/Else Statements");
}

static bool testEnumDeclarationStatement() {
    const std::string expression = "public enum Colour: int8 {\n"
                                   "    Red,\n"
                                   "    Green,\n"
                                   "    Blue,\n"
                                   "}\n"
                                   "private enum Status: float64 {\n"
                                   "    Success = 0,\n"
                                   "    Error = 1,\n"
                                   "    Unknown = -1,\n"
                                   "}"
                                   "enum Letters: uint128 {\n"
                                   "    A = 0,\n"
                                   "    B,\n"
                                   "    C,\n"
                                   "    D,\n"
                                   "    C,\n"
                                   "}";

    const std::array<std::string, 3> expected = {
        R"(public enum Colour: int8 {
    Red,
    Green,
    Blue
})",
        R"(private enum Status: float64 {
    Success = 0,
    Error = 1,
    Unknown = (-1)
})",
        R"(private enum Letters: uint128 {
    A = 0,
    B,
    C,
    D
})"};

    return validateStatements(getParserResults(expression), expected, "Enum Declaration Statement");
}

static bool testSwitchStatement() {
    const std::string expression = "switch (variable) {"
                                   "case 1:"
                                   "    print(\"One\");"
                                   "    ++i;"
                                   "case 2:"
                                   "    print(\"Two\");"
                                   "    --i;"
                                   "default:"
                                   "    print(\"Default case\");"
                                   "}";
    std::string expected = R"(switch (variable) {
    case 1:
        print("One");
        (++i);
    case 2:
        print("Two");
        (--i);
    default:
        print("Default case");
})";
    return validateStatement(getParserResults(expression), expected, "Switch Statement");
}

static bool testAccessExpressions() {
    const std::string expression = "let mut point = Point{x = 10, y = 20};\n"
                                   "let mut xCoord = point.x;\n"
                                   "let mut yCoord = point.y;\n"
                                   "let mut colour = rect.colour;"
                                   "let array = [1, 2, 3];\n"
                                   "let firstElement = array[0];\n"
                                   "let foo = lib::module_::function(a, b, c);\n";

    const std::array<std::string, 7> expected = {"(let mut point: private auto = Point {x = 10, y = 20});",
                                                 "(let mut xCoord: private auto = point.x);",
                                                 "(let mut yCoord: private auto = point.y);",
                                                 "(let mut colour: private auto = rect.colour);",
                                                 "(let array: private auto = [1, 2, 3]);",
                                                 "(let firstElement: private auto = array[0]);",
                                                 "(let foo: private auto = lib::module_::function(a, b, c));"};

    return validateStatements(getParserResults(expression), expected, "Member Access Expression");
}

static bool testGenerics() {
    const std::string expression = "func genericFunction[T, U, V](valueT: T, valueU: U, valueV: V) -> V {\n"
                                   "    return 3 + valueT + valueU * valueV;\n"
                                   "}\n"
                                   "let result = genericFunction@[int32, float64, char](5, 2.5, (65 as char));"
                                   "aggregate Foo[T, U] {\n"
                                   "    x: T;\n"
                                   "    y: U;\n"
                                   "}\n"
                                   "let foo = Foo@[int32, float64]{x = 3, y = 4.5};\n"
                                   "let foo_array: private Foo@[int32, float64][];";
    const std::array<std::string, 5> expected = {
        R"(private func genericFunction[T, U, V](valueT: T, valueU: U, valueV: V) -> V {
    return ((3 + valueT) + (valueU * valueV));
})",
        "(let result: private auto = genericFunction@[int32, float64, char](5, 2.5, (65 as char)));",
        R"(private aggregate Foo[T, U] {
    x: T;
    y: U;
})",
        "(let foo: private auto = Foo@[int32, float64] {x = 3, y = 4.5});",
        "(let foo_array: private Foo@[int32, float64][]);"};
    return validateStatements(getParserResults(expression), expected, "Generic Function Declaration");
}

static bool testImportsAndAliases() {
    const std::string expression = "import math::vector;\n"
                                   "import graphics::rendering as render;\n"
                                   "import std::collections::map;\n"
                                   "module dataprocessing;\n"
                                   "alias IntegerArray = int32[5];\n"
                                   "alias pf64 = ptr float64;\n"
                                   "alias blah = func(mut Integer, pf64, func(int64) -> int64) -> bool;\n"
                                   "alias StringIntMap = std::HashMap@[string, Integer];\n"
                                   "let value: Integer = 42;\n";

    const std::array<std::string, 6> expected
        = {"",
           "alias IntegerArray = (int32[5]);",
           "alias pf64 = (ptr float64);",
           "alias blah = (func(mut Integer, pf64, func(int64) -> int64) -> bool);",
           "alias StringIntMap = (std::HashMap@[string, Integer]);",
           "(let value: private Integer = 42);"};

    return validateStatements(getParserResults(expression), expected, "Import Statements and Type Aliases");
}

static bool testParseFromFile() {
    std::filesystem::path fullPath = std::filesystem::current_path() / "tests/parser_tests.mn";
    mnstl::chunk_allocator file_allocator{};
    parser::Parser p(fullPath.string(), lexer::Mode::File, file_allocator);
    auto x = p.parse();
    if (!x.moduleName.empty()) { std::cout << "module " << x.moduleName << ";\n"; }
    for (const auto& _import : x.imports) { std::cout << _import->toString() << "\n"; }

    for (const auto& element : x.program) { std::cout << element->toString(0) << "\n"; }
    return true;
}

static bool testRedundantSemicolons() {
    const std::string expression = "let x = 1 + 2;;;;;";
    const std::array<std::string, 5> expected = {"(let x: private auto = (1 + 2));", "", "", "", ""};
    return validateStatements(getParserResults(expression), expected, "Redundant Semicolons");
}

static bool testSizeofTypeofAlignof() {
    const std::string expression = "sizeof(int);\n"
                                   "sizeof(x+1);\n"
                                   "alignof(char);\n"
                                   "alignof(x+1);\n"
                                   "let x: typeof(x+1) = 3;\n"
                                   "let y : typeof(foo@[int,char]((p as int32)) + (bar + baz as typeof(3u128)));";
    const std::array<std::string, 6> expected
        = {"(sizeof(int32));",
           "(sizeof(dummy));",
           "(alignof(char));",
           "(alignof(dummy));",
           "(let x: private typeof((x + 1)) = 3);",
           "(let y: private typeof((foo@[int32, char]((p as int32)) + ((bar + baz) as typeof(3)))));"};
    return validateStatements(getParserResults(expression), expected, "Sizeof, Typeof & Alignof");
}

static bool testNestedBlocks() {
    const std::string expression
        = "func foo() {let x = 10; {let x = 20;} if (x == 10) {{let x = 10;}} else {{let x = 20;}}}";
    std::string expected = R"(private func foo() {
    (let x: private auto = 10);
    {
        (let x: private auto = 20);
    }
    if ((x == 10)) {
        {
            (let x: private auto = 10);
        }
    } else {
        {
            (let x: private auto = 20);
        }
    }
})";

    return validateStatement(getParserResults(expression), expected, "Nested Blocks");
}

static bool testNamespaces() {
    const std::string expression = "namespace Graphics {\n"
                                   "    public func drawPixel(x: int32, y: int32) {\n"
                                   "        print(\"Drawing pixel\");\n"
                                   "    }\n"
                                   "    aggregate Colour {\n"
                                   "        r: uint8;\n"
                                   "        g: uint8;\n"
                                   "        b: uint8;\n"
                                   "    }\n"
                                   "}\n"
                                   "namespace Math {\n"
                                   "    func magnitude(v: Vector) -> float64 {\n"
                                   "        return 0.0;\n"
                                   "    }\n"
                                   "}";

    const std::array<std::string, 2> expected = {
        R"(namespace Graphics {
    public func drawPixel(x: int32, y: int32) {
        print("Drawing pixel");
    }
    private aggregate Colour {
        r: uint8;
        g: uint8;
        b: uint8;
    }
})",
        R"(namespace Math {
    private func magnitude(v: Vector) -> float64 {
        return 0.0;
    }
})"};

    return validateStatements(getParserResults(expression), expected, "Namespace Declaration");
}

static bool miscTests() {
    const std::string expression = "let x = aggregate{1, \"asdf\", 3.1f32};";
    ast::Block x = getParserResults(expression);
    std::cout << x[0]->toString(0) << "\n";
    return true;
}

void runParserTests(TestRunner& runner) {
    std::ofstream logFile(logFileName, std::ios::trunc);
    logFile.close();

    runner.runTest("Arithmetic Expression and Casting", testArithmeticOperatorsAndCasting);
    runner.runTest("Variable Declaration", testVariableDeclaration);
    runner.runTest("Assignment Expressions", testAssignmentExpressions);
    runner.runTest("Prefix Operators", testPrefixOperators);
    runner.runTest("Parenthesized Expressions", testParenthesizedExpressions);
    runner.runTest("Address and Dereference Operators", testPointerOperators);
    runner.runTest("Typed Variable Declaration", testTypedVariableDeclaration);
    runner.runTest("Postfix Operators", testPostfixOperators);
    runner.runTest("Bitwise Operators", testBitwiseOperators);
    runner.runTest("Aggregate Declaration and Instantiation", testAggregateDeclarationAndInstantiation);
    runner.runTest("Function Declaration and Call", testFunctionDeclarationAndCall);
    runner.runTest("Loops", testLoops);
    runner.runTest("If/Elif/Else Statements", testIfElseStatements);
    runner.runTest("Enum Declaration Statement", testEnumDeclarationStatement);
    runner.runTest("Switch Statement", testSwitchStatement);
    runner.runTest("Access Expressions", testAccessExpressions);
    runner.runTest("Generics", testGenerics);
    runner.runTest("Imports and Type Aliases", testImportsAndAliases);
    runner.runTest("Parsing from file", testParseFromFile);
    runner.runTest("Redundant Semicolons", testRedundantSemicolons);
    runner.runTest("Sizeof, Typeof & Alignof", testSizeofTypeofAlignof);
    runner.runTest("Nested Blocks", testNestedBlocks);
    runner.runTest("Namespaces", testNamespaces);
    runner.runTest("Miscellaneous Tests", miscTests);
}
}  // namespace Manganese::tests