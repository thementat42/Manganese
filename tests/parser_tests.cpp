#include <iostream>
#include <memory>
#include <string>

#include "../src/frontend/include/parser.h"
#include "../src/global_macros.h"
#include "testrunner.h"

MANGANESE_BEGIN
namespace tests {

bool testArithmeticOperators() {
    // Test all arithmetic operators including their precedence and associativity
    std::string expression = "8 - 4 + 6 * 2 // 5 % 3 ** 2 ** 2 / 7;";
    parser::Parser parser(expression, lexer::Mode::String);

    // Parse the expression
    ast::Block block = parser.parse();

    // Print the parsed AST
    std::cout << "Parsed arithmetic operators AST:" << std::endl;
    for (const auto& stmt : block) {
        std::cout << stmt->toString() << std::endl;
    }

    // Check that we have exactly one statement
    if (block.size() != 1) {
        std::cerr << "ERROR: Expected 1 statement, got " << block.size() << std::endl;
        return false;
    }

    // Get the actual string representation of the AST
    std::string actual = block[0]->toString();

    std::string expected = "((8 - 4) + ((((6 * 2) // 5) % (3 ** (2 ** 2))) / 7));";

    // Compare the output with the expected string
    if (actual != expected) {
        std::cerr << "ERROR: AST does not match expected structure." << std::endl;
        std::cerr << "Expected: " << expected << std::endl;
        std::cerr << "Actual:   " << actual << std::endl;
        return false;
    }

    return true;
}

bool testExponentiationAssociativity() {
    // Test that exponentiation is right-associative:
    // 2 ** 3 ** 4 should be 2 ** (3 ** 4) = 2 ** 81 = a very large number
    // Not (2 ** 3) ** 4 = 8 ** 4 = 4096
    std::string expression = "2 ** 3 ** 4;";
    parser::Parser parser(expression, lexer::Mode::String);
    
    // Parse the expression
    ast::Block block = parser.parse();
    
    // Print the parsed AST
    std::cout << "Parsed exponentiation associativity AST:" << std::endl;
    for (const auto& stmt : block) {
        std::cout << stmt->toString() << std::endl;
    }
    
    // Check that we have exactly one statement
    if (block.size() != 1) {
        std::cerr << "ERROR: Expected 1 statement, got " << block.size() << std::endl;
        return false;
    }
    
    // Get the actual string representation of the AST
    std::string actual = block[0]->toString();
    
    // Define the expected string representation with right associativity
    std::string expected = "(2 ** (3 ** 4));";
    
    // Compare the output with the expected string
    if (actual != expected) {
        std::cerr << "ERROR: Exponentiation is not right-associative." << std::endl;
        std::cerr << "Expected: " << expected << std::endl;
        std::cerr << "Actual:   " << actual << std::endl;
        return false;
    }
    
    return true;
}

/*
bool testVariableDeclaration() {
    // Test parsing multiple variable declarations including float, variable reference, and const with comparison
    std::string expression = "let foo = 45.5;\nlet bar = foo * 10;\nconst baz = foo + 10 ** 2 * bar + foo % 7 + foo**2;";
    parser::Parser parser(expression, lexer::Mode::String);

    // Parse the expression
    ast::Block block = parser.parse();

    // Print the parsed AST
    std::cout << "Parsed multiple declarations AST:" << std::endl;
    for (const auto& stmt : block) {
        std::cout << stmt->toString() << std::endl;
    }

    // Check that we have exactly three statements
    if (block.size() != 3) {
        std::cerr << "ERROR: Expected 3 statements, got " << block.size() << std::endl;
        return false;
    }

    // Get the actual string representations of the AST
    std::string actual1 = block[0]->toString();
    std::string actual2 = block[1]->toString();
    std::string actual3 = block[2]->toString();

    // Define the expected string representations
    std::string expected1 = "(let foo = 45.5);";
    std::string expected2 = "(let bar = (foo * 10));";
    std::string expected3 = "(const baz = ((((foo + ((10 ** 2) * bar)) + (foo % 7)) + (foo ** 2))));";

    // Compare the output with the expected strings
    bool success = true;

    if (actual1 != expected1) {
        std::cerr << "ERROR: Float variable declaration not parsed correctly." << std::endl;
        std::cerr << "Expected: " << expected1 << std::endl;
        std::cerr << "Actual:   " << actual1 << std::endl;
        success = false;
    }

    if (actual2 != expected2) {
        std::cerr << "ERROR: Variable reference declaration not parsed correctly." << std::endl;
        std::cerr << "Expected: " << expected2 << std::endl;
        std::cerr << "Actual:   " << actual2 << std::endl;
        success = false;
    }

    if (actual3 != expected3) {
        std::cerr << "ERROR: Complex declaration not parsed correctly." << std::endl;
        std::cerr << "Expected: " << expected3 << std::endl;
        std::cerr << "Actual:   " << actual3 << std::endl;
        success = false;
    }

    return success;
}

bool testAssignmentExpressions() {
    // Test parsing assignment expressions including compound assignments
    std::string expression =
        "a = 5;\n"
        "b += 3;\n"
        "c -= 2 * d;\n"
        "e *= f + 1;\n"
        "g /= h - 2;\n"
        "i %= 4;\n"
        "j **= 2;\n"
        "k //= 3;";
    parser::Parser parser(expression, lexer::Mode::String);

    // Parse the expression
    ast::Block block = parser.parse();

    // Print the parsed AST
    std::cout << "Parsed assignment expressions AST:" << std::endl;
    for (const auto& stmt : block) {
        std::cout << stmt->toString() << std::endl;
    }

    // Check that we have exactly eight statements
    if (block.size() != 8) {
        std::cerr << "ERROR: Expected 8 statements, got " << block.size() << std::endl;
        return false;
    }

    // Define the expected string representations
    std::string expected[] = {
        "(a = 5);",
        "(b += 3);",
        "(c -= (2 * d));",
        "(e *= (f + 1));",
        "(g /= (h - 2));",
        "(i %= 4);",
        "(j **= 2);",
        "(k //= 3);"};

    bool success = true;
    for (size_t i = 0; i < 8; ++i) {
        std::string actual = block[i]->toString();
        if (actual != expected[i]) {
            std::cerr << "ERROR: Assignment expression " << (i + 1) << " not parsed correctly." << std::endl;
            std::cerr << "Expected: " << expected[i] << std::endl;
            std::cerr << "Actual:   " << actual << std::endl;
            success = false;
        }
    }

    return success;
}

bool testPrefixOperators() {
    // Test parsing prefix operators: ++, --, -, +, !
    std::string expression =
        "++x;\n"
        "--y;\n"
        "-z;\n"
        "+a;\n"
        "!b;\n"
        // "-(d + 3);"
        "++c * 2;\n";
    parser::Parser parser(expression, lexer::Mode::String);

    // Parse the expression
    ast::Block block = parser.parse();

    // Print the parsed AST
    std::cout << "Parsed prefix operators AST:" << std::endl;
    for (const auto& stmt : block) {
        std::cout << stmt->toString() << std::endl;
    }

    // Check that we have exactly seven statements
    if (block.size() != 6) {
        std::cerr << "ERROR: Expected 6 statements, got " << block.size() << std::endl;
        return false;
    }

    // Define the expected string representations
    std::string expected[] = {
        "(++x);",
        "(--y);",
        "(-z);",
        "(+a);",
        "(!b);",
        // "(-(d + 3));"
        "((++c) * 2);",
    };

    bool success = true;
    for (size_t i = 0; i < 6; ++i) {
        std::string actual = block[i]->toString();
        if (actual != expected[i]) {
            std::cerr << "ERROR: Prefix operator expression " << (i + 1) << " not parsed correctly." << std::endl;
            std::cerr << "Expected: " << expected[i] << std::endl;
            std::cerr << "Actual:   " << actual << std::endl;
            success = false;
        }
    }

    return success;
}
*/

int runParserTests(TestRunner& runner) {
    runner.runTest("Simple Arithmetic Expression", testArithmeticOperators);
    runner.runTest("Exponentiation Right Associativity", testExponentiationAssociativity);
    // runner.runTest("Variable Declaration", testVariableDeclaration);
    // runner.runTest("Assignment Expressions", testAssignmentExpressions);
    // runner.runTest("Prefix Operators", testPrefixOperators);

    return runner.allTestsPassed() ? 0 : 1;
}
}  // namespace tests
MANGANESE_END