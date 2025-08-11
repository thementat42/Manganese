/**
 * @file parser_tests.cpp
 * @brief Unit tests for the parser.
 *
 * Each test uses helper functions to parse some source code into an AST
 * the resulting string representation for correctness. The tests are registered and run
 * via a TestRunner instance.
 *
 * @see include/frontend/parser.h
 * @see testrunner.h
 */

#include <frontend/parser.hpp>
#include <global_macros.hpp>

#include <array>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "testrunner.hpp"

// NOTE: In the parser, any variable declaration without an explicit type is marked as 'auto'
// The semantic analysis phase is responsible for resolving the actual type
// So testing for a correct type resolution is outside the scope of these tests

namespace Manganese {
namespace tests {

static const char* logFileName = "logs/parser_tests.log";

ast::Block getParserResults(const std::string& source, lexer::Mode mode = lexer::Mode::String) {
    parser::Parser parser(source, mode);
    if (parser.hasCriticalError()) { throw std::runtime_error("Compilation Aborted\n"); }
    parser::ParsedFile file = parser.parse();

    if (!file.moduleName.empty()) { std::cout << "module " << file.moduleName << "\n"; }
    if (!file.imports.empty()) {
        for (const auto& element : file.imports) { std::cout << parser::importToString(element) << "\n"; }
    }

    if (!file.blockComments.empty()) {
        std::cout << "Block comments:\n";
        for (const auto& comment : file.blockComments) { std::cout << comment << '\n'; }
    }

    return std::move(file.program);
}

template <size_t N>
bool validateStatements(const ast::Block& block, const std::array<std::string, N>& expected, const char* testName) {
    std::ofstream logFile(logFileName, std::ios::app);
    if (!logFile) {
        std::cerr << "ERROR: Could not open log file for writing.\n";
    } else {
        logFile << "Test: " << testName << '\n';
    }
    std::cout << "Parsed " << testName << " AST:" << '\n';
    for (const auto& stmt : block) {
        std::cout << stmt->toString() << '\n';
        if (logFile) {
            logFile << "String representation: " << stmt->toString() << '\n';
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

    for (size_t i = 0; i < N; ++i) {
        std::string actual = block[i]->toString();
        if (actual != expected[i]) {
            std::cerr << "ERROR: Statement " << (i + 1) << " does not match expected in test: " << testName << '\n';
            std::cerr << "Expected: " << expected[i] << '\n';
            std::cerr << "Actual:   " << actual << '\n';
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
        std::cout << stmt->toString() << '\n';
        if (logFile) {
            logFile << "String representation: " << stmt->toString() << '\n';
            logFile << "Dumping statement:\n";
            stmt->dump(logFile);
            logFile << "---------------------\n";
        }
    }

    if (block.size() != 1) {
        std::cerr << "ERROR: Expected 1 statement, got " << block.size() << " in test: " << testName << '\n';
        return false;
    }

    std::string actual = block[0]->toString();
    if (actual != expected) {
        std::cerr << "ERROR: Statement does not match expected in test: " << testName << '\n';
        std::cerr << "Expected: " << expected << '\n';
        std::cerr << "Actual:   " << actual << '\n';
        return false;
    }

    return true;
}

bool testArithmeticOperatorsAndCasting() {
    // Test all arithmetic operators including their precedence and associativity
    // Also test type casting with 'as' operator
    std::string expression = "8 - 4 + 6 * 2 // 5 % 3 ^^ 2 ^^ 2 / 7 as float32;";
    std::string expected = "(((8 - 4) + ((((6 * 2) // 5) % (3 ^^ (2 ^^ 2))) / 7)) as float32);";

    return validateStatement(getParserResults(expression), expected, "Arithmetic Operators and Casting");
}

bool testExponentiationAssociativity() {
    // Test that exponentiation is right-associative:
    // i.e. Evaluate the exponent before grouping it with the base
    std::string expression = "2 ^^ 3 ^^ 2;";
    std::string expected = "(2 ^^ (3 ^^ 2));";

    return validateStatement(getParserResults(expression), expected, "Exponentiation Associativity");
}

bool testVariableDeclaration() {
    std::string expression = "let foo = 45.5;"
                             "let bar = foo * 10;"
                             "const baz : public uint32 = foo + 10 ^^ 2 * bar + foo % 7 + foo^^2;"
                             "let boolean = true;";

    std::array<std::string, 4> expected
        = {"(let foo: private auto = 45.5);", "(let bar: private auto = (foo * 10));",
           "(const baz: public uint32 = (((foo + ((10 ^^ 2) * bar)) + (foo % 7)) + (foo ^^ 2)));",
           "(let boolean: private auto = true);"};

    return validateStatements(getParserResults(expression), expected, "Variable Declaration");
}

bool testAssignmentExpressions() {
    std::string expression = "a = 5;\n"
                             "b += 3;\n"
                             "c -= 2 * b;\n"
                             "d = -(c + 3);\n"
                             "e *= f + 1;\n"
                             "g /= h - -2;\n"
                             "i %= 4;\n"
                             "j ^^= 2;\n"
                             "k //= 3;"
                             "l = (3 + 4) * 2 - (1 + 1) ^^ 5;"
                             "a &= b;\n"
                             "c |= d;\n"
                             "e ^= f;\n"
                             "g <<= 2;\n"
                             "h >>= 3;\n"
                             "i &= j | k;\n"
                             "m |= n & p;\n"
                             "x ^= ~y;\n";

    std::array<std::string, 18> expected = {"(a = 5);",        "(b += 3);",
                                            "(c -= (2 * b));", "(d = (-(c + 3)));",
                                            "(e *= (f + 1));", "(g /= (h - (-2)));",
                                            "(i %= 4);",       "(j ^^= 2);",
                                            "(k //= 3);",      "(l = (((3 + 4) * 2) - ((1 + 1) ^^ 5)));",
                                            "(a &= b);",       "(c |= d);",
                                            "(e ^= f);",       "(g <<= 2);",
                                            "(h >>= 3);",      "(i &= (j | k));",
                                            "(m |= (n & p));", "(x ^= (~y));"};

    return validateStatements(getParserResults(expression), expected, "Assignment Expressions");
}

bool testPrefixOperators() {
    std::string expression = "++x;\n"
                             "--y;\n"
                             "-z;\n"
                             "+a;\n"
                             "!b;\n"
                             "-(d + 3);"
                             "++c * 2;\n";

    std::array<std::string, 7> expected
        = {"(++x);", "(--y);", "(-z);", "(+a);", "(!b);", "(-(d + 3));", "((++c) * 2);"};

    return validateStatements(getParserResults(expression), expected, "Prefix Operators");
}

bool testParenthesizedExpressions() {
    // Parentheses should override operator precedence
    std::string expression = "(2 + 3) * 4;\n"
                             "2 * (3 + 4);\n"
                             "((5 + 2) * (8 - 3)) / 2;\n"
                             "1 + (2 ^^ (3 + 1));\n"
                             "((2 + 3) * 4) - (6 / (1 + 1));";

    std::array<std::string, 5> expected = {"((2 + 3) * 4);", "(2 * (3 + 4));", "(((5 + 2) * (8 - 3)) / 2);",
                                           "(1 + (2 ^^ (3 + 1)));", "(((2 + 3) * 4) - (6 / (1 + 1)));"};

    return validateStatements(getParserResults(expression), expected, "Parenthesized Expressions");
}

bool testPointerOperators() {
    // Test that & and * get correctly interpreted as pointer operators
    std::string expression = "&variable;\n"
                             "*pointer;\n"
                             "**doublePointer;\n"
                             "&(x + y);\n"
                             "*p + 5;\n";

    std::array<std::string, 5> expected
        = {"(&variable);", "(*pointer);", "(*(*doublePointer));", "(&(x + y));", "((*p) + 5);"};

    return validateStatements(getParserResults(expression), expected, "Pointer Operators");
}

bool testTypedVariableDeclaration() {
    std::string expression = "let x: int32 = 42;\n"
                             "const y: public float64 = 3.14159;\n"
                             "let z: char = 'A';\n"
                             "let numbers: int32[3^^2];\n"
                             "const matrix: readonly float32[][] = [[1.0, 2.7], [3.0, 4.2]];\n";

    std::array<std::string, 5> expected = {"(let x: private int32 = 42);", "(const y: public float64 = 3.14159);",
                                           "(let z: private char = 'A');", "(let numbers: private int32[(3 ^^ 2)]);",
                                           "(const matrix: readonly float32[][] = [[1.0, 2.7], [3.0, 4.2]]);"};

    return validateStatements(getParserResults(expression), expected, "Typed Variable Declarations");
}

bool testPostfixOperators() {
    std::string expression = "x++;\n"
                             "y--;\n"
                             "(a + b)++;\n"
                             "arr[i]--;\n"
                             "++x--;\n"
                             "x++ + y--;\n";

    std::array<std::string, 6> expected = {"(x++);",          "(y--);", "((a + b)++);", "(arr[i]--);",
                                           "(++(x--));",  // Postfix should be applied first, then prefix
                                           "((x++) + (y--));"};

    return validateStatements(getParserResults(expression), expected, "Postfix Operators");
}

bool testBitwiseOperators() {
    std::string expression = "a & b;\n"
                             "c | d;\n"
                             "e ^ f;\n"
                             "~g;\n"
                             "h << 2;\n"
                             "i >> 3;\n"
                             "(a & b) | (c ^ d);\n"
                             "a & (b | c);\n"
                             "~(a & b) | c;\n"
                             "a & b & c | d ^ e;\n";

    std::array<std::string, 10> expected = {"(a & b);",
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

bool testAggregateDeclarationAndInstantiation() {
    std::string expression = "public aggregate Point {\n"
                             "    x: int32;\n"
                             "    y: int32;\n"
                             "    some_field: float64;\n"
                             "}\n"
                             "aggregate Rectangle {\n"
                             "    topLeft: Point;\n"
                             "    bottomRight: Point;\n"
                             "    color: uint32;\n"
                             "}\n"
                             "let p1 = Point{x = 10, y = 20};\n"
                             "let p2: Point = Point{x = 30, y = 40};\n"
                             "const rect = Rectangle{\n"
                             "    topLeft = Point{x = 0, y = 0},\n"
                             "    bottomRight = p2,\n"
                             "    color = 0xFF0000\n"
                             "};\n";

    // Note: In the final declaration, the numeric value for the colour is the decimal equivalent of 0xFF0000
    std::array<std::string, 5> expected = {
        "public aggregate Point {\n\tx: int32;\n\ty: int32;\n\tsome_field: float64;\n}",
        "private aggregate Rectangle {\n\ttopLeft: Point;\n\tbottomRight: Point;\n\tcolor: uint32;\n}",
        "(let p1: private auto = Point {x = 10, y = 20});", "(let p2: private Point = Point {x = 30, y = 40});",
        "(const rect: private auto = Rectangle {topLeft = Point {x = 0, y = 0}, bottomRight = p2, color = 16711680});"};

    return validateStatements(getParserResults(expression), expected, "Aggregate Declaration and Instantiation");
}

bool testFunctionDeclarationAndCall() {
    std::string expression = "readonly func add(a: int32, b: int32) -> int32 {\n"
                             "    return a + b;\n"
                             "}\n"
                             "func greet(name: string) {\n"
                             "    print(\"Hello, \" + name);\n"
                             "}\n"
                             "func calculate(x: float64, y: const float64) -> float64 {\n"
                             "    let result = x * y;\n"
                             "    return result;\n"
                             "}\n"
                             "let sum = add(5u32, 3i16);\n"
                             "greet(\"World\");\n"
                             "let product = calculate(2.5f64, 3.01);\n";

    std::array<std::string, 6> expected = {
        "readonly func add(a: int32, b: int32) -> int32 {\nreturn (a + b);\n}",
        "private func greet(name: string) {\nprint((\"Hello, \" + name));\n}",
        "private func calculate(x: float64, y: const float64) -> float64 {\n(let result: private auto = (x * y));\nreturn result;\n}",
        "(let sum: private auto = add(5, 3));",
        "greet(\"World\");",
        "(let product: private auto = calculate(2.5, 3.01));"};

    return validateStatements(getParserResults(expression), expected, "Function Declaration and Call");
}

bool testLoops() {
    std::string expression = "let i = 0;"
                             "do {++i; print(i); } while (i < 5);"
                             "let j: int32 = 10;"
                             "while (true) {"
                             "    if (j == 5) {continue;}"
                             "    print(j--);"
                             "    if (j <= 0) { break; }"
                             "}"
                             "repeat ((5 + 30 - 2 ^^ 3) << 2) {print(\"Hello\");}";

    std::array<std::string, 5> expected = {
        "(let i: private auto = 0);", "do {\n\t(++i);\n\tprint(i);\n} while ((i < 5));", "(let j: private int32 = 10);",
        "while (true) {\n\tif ((j == 5)) {\n\tcontinue;\n}\n\tprint((j--));\n\tif ((j <= 0)) {\n\tbreak;\n}\n}",
        "repeat ((((5 + 30) - (2 ^^ 3)) << 2)) {\n\tprint(\"Hello\");\n}"};

    return validateStatements(getParserResults(expression), expected, "Loops");
}

bool testIfElseStatements() {
    std::string expression = "if (a < b) {\n"
                             "    let result = a + b;\n"
                             "    print(result);\n"
                             "} elif (a > b) {\n"
                             "    let result = a - b;\n"
                             "    print(result);\n"
                             "} else {\n"
                             "    print(\"Equal\");\n"
                             "}";

    std::string expected = "if ((a < b)) {\n"
                           "\t(let result: private auto = (a + b));\n"
                           "\tprint(result);\n"
                           "} elif ((a > b)) {\n"
                           "\t(let result: private auto = (a - b));\n"
                           "\tprint(result);\n"
                           "} else {\n"
                           "\tprint(\"Equal\");\n"
                           "}";

    return validateStatement(getParserResults(expression), expected, "If/Else If/Else Statements");
}

bool testEnumDeclarationStatement() {
    std::string expression = "public enum Color {\n"
                             "    Red,\n"
                             "    Green,\n"
                             "    Blue,\n"
                             "}\n"
                             "readonly enum Status: float64 {\n"
                             "    Success = 0,\n"
                             "    Error = 1,\n"
                             "    Unknown = -1,\n"
                             "}";

    std::array<std::string, 2> expected
        = {"public enum Color: int32 {\n\tRed,\n\tGreen,\n\tBlue,\n}",
           "private enum Status: float64 {\n\tSuccess = 0,\n\tError = 1,\n\tUnknown = (-1),\n}"};

    return validateStatements(getParserResults(expression), expected, "Enum Declaration Statement");
}

bool testSwitchStatement() {
    std::string expression = "switch (variable) {"
                             "case 1:"
                             "    print(\"One\");"
                             "    ++i;"
                             "case 2:"
                             "    print(\"Two\");"
                             "    --i;"
                             "default:"
                             "    print(\"Default case\");"
                             "}";
    std::string expected = "switch (variable) {\n"
                           "\tcase 1:\n"
                           "\t\tprint(\"One\");\n"
                           "\t\t(++i);\n"
                           "\tcase 2:\n"
                           "\t\tprint(\"Two\");\n"
                           "\t\t(--i);\n"
                           "\tdefault:\n"
                           "\t\tprint(\"Default case\");\n"
                           "}";
    return validateStatement(getParserResults(expression), expected, "Switch Statement");
}

bool testAccessExpressions() {
    std::string expression = "let point = Point{x = 10, y = 20};\n"
                             "let xCoord = point.x;\n"
                             "let yCoord = point.y;\n"
                             "let color = rect.color;"
                             "const array = [1, 2, 3];\n"
                             "let firstElement = array[0];\n"
                             "let foo = lib::module_::function(a, b, c);\n";

    std::array<std::string, 7> expected = {"(let point: private auto = Point {x = 10, y = 20});",
                                           "(let xCoord: private auto = point.x);",
                                           "(let yCoord: private auto = point.y);",
                                           "(let color: private auto = rect.color);",
                                           "(const array: private auto = [1, 2, 3]);",
                                           "(let firstElement: private auto = array[0]);",
                                           "(let foo: private auto = lib::module_::function(a, b, c));"};

    return validateStatements(getParserResults(expression), expected, "Member Access Expression");
}

bool testGenerics() {
    std::string expression = "func genericFunction[T, U, V](valueT: T, valueU: U, valueV: V) -> V {\n"
                             "    return 3 + valueT + valueU * valueV;\n"
                             "}\n"
                             "let result = genericFunction@[int32, float64, char](5, 2.5, (65 as char));"
                             "aggregate Foo[T, U] {\n"
                             "    x: T;\n"
                             "    y: U;\n"
                             "}\n"
                             "let foo = Foo@[int32, float64]{x = 3, y = 4.5};\n"
                             "let foo_array: readonly Foo@[int32, float64][];";
    std::array<std::string, 5> expected = {
        "private func genericFunction[T, U, V](valueT: T, valueU: U, valueV: V) -> V {\nreturn ((3 + valueT) + (valueU * valueV));\n}",
        "(let result: private auto = genericFunction@[int32, float64, char](5, 2.5, (65 as char)));",
        "private aggregate Foo[T, U] {\n\tx: T;\n\ty: U;\n}",
        "(let foo: private auto = Foo@[int32, float64] {x = 3, y = 4.5});",
        "(let foo_array: readonly Foo@[int32, float64][]);"};
    return validateStatements(getParserResults(expression), expected, "Generic Function Declaration");
}

bool testImportsAndAliases() {
    std::string expression = "import math::vector;\n"
                             "import graphics::rendering as render;\n"
                             "import std::collections::map;\n"
                             "module dataprocessing;\n"
                             "alias int32 as Integer;\n"
                             "alias ptr float64 as pf64;\n"
                             "alias func(const Integer, pf64, func(int64) -> int64) -> bool as blah;"
                             "alias std::HashMap@[string, Integer] as StringIntMap;\n"
                             "let value: Integer = 42;\n";

    std::array<std::string, 6> expected = {"",
                                           "alias (int32) as Integer;",
                                           "alias (ptr float64) as pf64;",
                                           "alias (func(const Integer, pf64, func(int64) -> int64) -> bool) as blah;",
                                           "alias (std::HashMap@[string, Integer]) as StringIntMap;",
                                           "(let value: private Integer = 42);"};

    return validateStatements(getParserResults(expression), expected, "Import Statements and Type Aliases");
}

bool testParseFromFile() {
    std::filesystem::path fullPath = std::filesystem::current_path() / "tests/parser_tests.mn";
    parser::Parser p(fullPath.string(), lexer::Mode::File);
    auto x = p.parse();
    if (!x.moduleName.empty()) { std::cout << "module " << x.moduleName << ";\n"; }
    for (const auto& element : x.imports) { std::cout << parser::importToString(element) << "\n"; }

    for (const auto& element : x.program) { std::cout << element->toString() << "\n"; }
    // For now, this is a more manual check -- see if the output makes sense
    return true;
}

bool testRedundantSemicolons() {
    std::string expression = "let x = 1 + 2;;;;;";
    std::array<std::string, 5> expected = {"(let x: private auto = (1 + 2));", "", "", "", ""};
    return validateStatements(getParserResults(expression), expected, "Redundant Semicolons");
}

static bool miscTests() {
    std::string expression = "let x: (ptr int)[];";
    auto x = getParserResults(expression);
    std::cout << x[0]->toString() << "\n";
    x[0]->dump(std::cout);
    return true;
}

int runParserTests(TestRunner& runner) {
    // Clear the log file before running tests
    std::ofstream logFile(logFileName, std::ios::trunc);
    logFile.close();  // Here, we don't really care if the clearing failed

    runner.runTest("Arithmetic Expression and Casting", testArithmeticOperatorsAndCasting);
    runner.runTest("Exponentiation Right Associativity", testExponentiationAssociativity);
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
    runner.runTest("Miscellaneous Tests", miscTests);

    return runner.allTestsPassed() ? 0 : 1;
}
}  // namespace tests
}  // namespace Manganese