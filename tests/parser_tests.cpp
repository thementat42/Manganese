#include "../src/frontend/include/parser.h"
#include "../src/global_macros.h"
#include "testrunner.h"
#include <iostream>
#include <memory>
#include <string>

namespace manganese {
namespace tests {

bool testSimpleArithmeticExpression() {
    // Test parsing a simple arithmetic expression: "3 + 5 * 7 + 9 - 2"
    std::string expression = "3 + 5 * 7 + 9 - 2;";
    parser::Parser parser(expression, lexer::Mode::String);
    
    // Parse the expression
    ast::Block block = parser.parse();
    
    // Print the parsed AST
    std::cout << "Parsed expression AST:" << std::endl;
    for (const auto& stmt : block) {
        std::cout << stmt->toString() << std::endl;
    }
      // Verify that the AST represents the expression with correct operator precedence
    // The multiplication (5 * 7) should be evaluated first, then the additions and subtractions from left to right
    // Expected structure: ((3 + (5 * 7)) + 9) - 2
    
    // Check that we have exactly one statement
    if (block.size() != 1) {
        std::cerr << "ERROR: Expected 1 statement, got " << block.size() << std::endl;
        return false;
    }
    
    // Get the actual string representation of the AST
    std::string actual = block[0]->toString();
    
    // Define the expected string representation with proper operator precedence
    // The exact format depends on your AST's toString() implementation
    std::string expected = "(((3 + (5 * 7)) + 9) - 2);";

    // Compare the output with the expected string
    if (actual != expected) {
        std::cerr << "ERROR: AST does not match expected structure." << std::endl;
        std::cerr << "Expected: " << expected << std::endl;
        std::cerr << "Actual:   " << actual << std::endl;
        return false;
    }
    
    return true;
}

int runParserTests(TestRunner& runner) {
    // Register the test
    runner.runTest("Simple Arithmetic Expression", testSimpleArithmeticExpression);
    
    return 0;
}
}  // namespace tests
}  // namespace manganese