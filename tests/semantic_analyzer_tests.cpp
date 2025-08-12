/**
 * @file semantic_analyzer_tests.cpp
 * @brief Unit tests for the semantic_analyzer.
 *
 * @see include/frontend/semantic_analyzer.h
 * @see testrunner.h
 */

#include <frontend/ast.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic.hpp>
#include <global_macros.hpp>
#include <iostream>

#include "frontend/parser/parser_base.hpp"
#include "frontend/semantic/semantic_analyzer.hpp"
#include "testrunner.hpp"

//  NOTE: For now, these tests will "always pass" (i.e., there's no automatic checking of the semantic analyzer's ouput)
//  The tests just call dump() on the resulting ast nodes
//  TODO: Implement more automated checking of the output

namespace Manganese {

namespace tests {
static const char* logFileName = "logs/semantic_analyzer_tests.log";

inline constexpr bool dumpASTToStdout
    = false;  // The nodes are always dumped to a log file -- this controls whether they are also dumped to stdout

parser::ParsedFile parse(const std::string& source, lexer::Mode mode = lexer::Mode::String) {
    parser::Parser parser(source, mode);
    return parser.parse();
}

void outputAnalyzedAST(const ast::Block& program) {
    std::ofstream logFile(logFileName, std::ios::app);
    if (!logFile) { std::cerr << "ERROR: Could not open log file for writing.\n"; }
    std::cout << "Analyzed AST:\n";
    for (const auto& statement : program) {
        std::cout << "String representation: " << statement->toString() << "\n";
        if (dumpASTToStdout) {
            std::cout << "Dumped statement:\n";
            statement->dump(std::cout);
        }
        if (logFile) {
            logFile << "String representation: " << statement->toString() << "\n";
            logFile << "Dumped statement:\n";
            statement->dump(logFile);
            logFile << "---------------------\n";
        }
    }
}

bool analyzeLiterals() {
    semantic::SemanticAnalyzer analyzer;
    parser::ParsedFile file = parse("true; \"asdf\"; [1i64, 2, 3, 4, true]; 100; 'a';");
    analyzer.analyze(file);
    const ast::Block& program = file.program;
    outputAnalyzedAST(program);
    if (program.size() != 5) {
        std::cerr << "Expected 5 statements, got " << program.size() << "\n";
        return false;
    }

    return true;
}

bool analyzeSimpleVariableDeclaration() {
    semantic::SemanticAnalyzer analyzer;
    parser::ParsedFile file = parse("let x: int64 = 10; let x: int8 = 20i32;"
                                    "let y: string = \"hello\";"
                                    "const z = x; z = 3;"
                                    "let a = [[1, 2], [3, 4]]; const b = a[0]; let c = b[1];"
                                    "a[1] = 0; # Should fail\n"
                                    "a[0] = [1, 1];"
                                    "a[0][1] = 99;"
                                    "b[0] = 3;");
    analyzer.analyze(file);
    const auto& program = file.program;
    outputAnalyzedAST(program);
    if (program.size() != 12) {
        std::cerr << "Expected 21 statements, got " << program.size() << "\n";
        return false;
    }
    return true;
}

bool analyzeAliases() {
    semantic::SemanticAnalyzer analyzer;
    parser::ParsedFile file = parse("alias int64 as i64;"
                                    "alias float64 as f64;"
                                    "alias string as foo;"
                                    "alias bar as foo;");
    analyzer.analyze(file);
    const auto& program = file.program;
    outputAnalyzedAST(program);
    if (program.size() != 4) {
        std::cerr << "Expected 4 statements, got " << program.size() << "\n";
        return false;
    }
    return true;
}

bool analyzeAggregateInstantiation() {
    semantic::SemanticAnalyzer analyzer;
    parser::ParsedFile file = parse("aggregate Point3D {"
                                    "  x: int64;"
                                    "  y: int64;"
                                    "  z: int64;"
                                    "}"
                                    "let p = Point3D{ x = 1, y = 2, z = 3 };"
                                    "let q = Point3D{ x = \"Hi!\", z = 5, y = 6 };");
    analyzer.analyze(file);
    const auto& program = file.program;
    outputAnalyzedAST(program);
    if (program.size() != 3) {
        std::cerr << "Expected 3 statements, got " << program.size() << "\n";
        return false;
    }
    return true;
}

bool analyzeFunctionDeclarationAndCall() {
    semantic::SemanticAnalyzer analyzer;
    parser::ParsedFile file = parse("alias int64 as i64;"
                                    "func foo(a: i64, b: i64) -> i64 { return a; }"
                                    "let x = foo;"
                                    "let result = x(5, 10);"
                                    "func greet(name: string) { print(\"Hello, \" + name); }"
                                    "greet(\"World\");");
    analyzer.analyze(file);
    const auto& program = file.program;
    outputAnalyzedAST(program);
    if (program.size() != 6) {
        std::cerr << "Expected 6 statements, got " << program.size() << "\n";
        return false;
    }
    return true;
}

bool testBinaryExpressions() {
    semantic::SemanticAnalyzer analyzer;
    parser::ParsedFile file = parse("let a = 8 + 3i64 * 12 - 4 ^^ 8;"
                                    "let b = 8 - 3i64;"
                                    "let c = \"Hello!\"*10u32;"
                                    "let d = 4/2;"
                                    "let e = 4//2;"
                                    "let f = 3%5;"
                                    "let g = 5 < 4;"
                                    "let h = 6 > (0 - 2);"
                                    "let i = 3 <= 1;"
                                    "let j = 8 >= 4;"
                                    "let k = 1 == 1 * 1;"
                                    "let l = 7 != (5*3);"
                                    "let m = true || (false && false);"
                                    "let n = 1 & 3;"
                                    "let o = 1 | 3;"
                                    "let p = 1 ^ 3;"
                                    "let q = 1 << 3;"
                                    "let r = 1 >> 3;"
                                    "let s = [a, b] + [c, d];"
                                    "let t = [e, f] * 10i8;"
                                    "let u = [g, h] > [i, j];");

    analyzer.analyze(file);
    const auto& program = file.program;
    outputAnalyzedAST(program);
    if (program.size() != 21) {
        std::cerr << "Expected 21 statements, got " << program.size() << "\n";
        return false;
    }
    return true;
}

bool checkMemberAccessExpression() {
    semantic::SemanticAnalyzer analyzer;
    parser::ParsedFile file = parse("aggregate A {b: int;}"
                                    "aggregate B {c : A;}"
                                    "let a = B{c = A{b = 2}};"
                                    "let x = a.c.b;"
                                    "let y = a.x;  # check non-existent member access\n"
                                    "let z = a.c.b + 3 ^^ 4;");
    analyzer.analyze(file);
    const auto& program = file.program;
    outputAnalyzedAST(program);
    if (program.size() != 6) {
        std::cerr << "Expected 6 statements, got " << program.size() << "\n";
        return false;
    }
    return true;
}

bool checkTypeCastExpression() {
    semantic::SemanticAnalyzer analyzer;

    parser::ParsedFile file = parse("let x: int64 = 10;"
                                    "let y: float32 = 3.14;"
                                    "let z = x as float32;"
                                    "let w = y as int64;"
                                    "let a = \"Hello\" as char;"  // Should be disallowed
                                    "let b = 42 * 4 as int8 + (5 as float32);");
    analyzer.analyze(file);
    const auto& program = file.program;
    outputAnalyzedAST(program);
    if (program.size() != 6) {
        std::cerr << "Expected 6 statements, got " << program.size() << "\n";
        return false;
    }
    return true;
}

bool checkLoops() {
    semantic::SemanticAnalyzer analyzer;
    parser::ParsedFile file = parse(
        R"(
        func foo(x: const int, y: const int) -> int {
            let q = 1;
            while (q*x*y <= 100) {
            q = q + 1;
            x = x + 1;  # Should have an error
        }
        return q;
    }
    let some_var = foo(10, 3 + 4^^2);

    )");
    analyzer.analyze(file);
    const auto& program = file.program;
    outputAnalyzedAST(program);
    if (program.size() != 2) {
        std::cerr << "Expected 2 statements, got " << program.size() << "\n";
        return false;
    }
    return true;
}

bool checkPrefixAndPostfixExpressions() {
    // Function call and index expressions are also postfix expressions, but they are handled separately
    semantic::SemanticAnalyzer analyzer;
    parser::ParsedFile file = parse(
        R"(
        let x = 10;
        let y = true;

        ++x;
        x++;
        --x;
        x--;
        !y;
        +x;
        -x;
        ~x;
        let p_x = &x;
        let x2 = *p_x;
        )");
    analyzer.analyze(file);
    const auto& program = file.program;
    outputAnalyzedAST(program);
    if (program.size() != 12) {
        std::cerr << "Expected 12 statements, got " << program.size() << "\n";
        return false;
    }
    return true;
}

static bool miscTests() {
    // TODO: Consider immutability by default
    //! Functions can be assigned to each other (no way to prevent this)
    //! This is weird
    //! Having just functions be immutable by default would be unintuitive
    semantic::SemanticAnalyzer analyzer;
    parser::ParsedFile file = parse(
        R"(
        func foo() -> int {return 3;}
        func bar() -> int {return 1;}
        let x: int[10] = [1,2,3,4,5,6,7,8,9,10];
        3 = 10;
        x[0] = 5;
        x[0][0] = 100;
        foo = bar;
        foo() = 3;
        bar() = [3];
        bar()[0] = 3;
        )");
    analyzer.analyze(file);
    const auto& program = file.program;
    outputAnalyzedAST(program);
    return true;
}

void runSemanticAnalysisTests(TestRunner& runner) {
    // Clear the log file before running tests
    std::ofstream logFile(logFileName, std::ios::trunc);
    logFile.close();  // Here, we don't really care if the clearing failed

    runner.runTest("Analyze Literals", analyzeLiterals);
    runner.runTest("Analyze Simple Variable Declaration", analyzeSimpleVariableDeclaration);
    runner.runTest("Analyze Aliases", analyzeAliases);
    runner.runTest("Analyze aggregate Instantiation", analyzeAggregateInstantiation);
    runner.runTest("Analyze Function Declaration and Call", analyzeFunctionDeclarationAndCall);
    runner.runTest("Binary Expressions", testBinaryExpressions);
    runner.runTest("Member Access", checkMemberAccessExpression);
    runner.runTest("Type Cast Expression", checkTypeCastExpression);
    runner.runTest("Analyze Loops", checkLoops);
    runner.runTest("Analyze Prefix and Postfix Expressions", checkPrefixAndPostfixExpressions);
    runner.runTest("Mist Tests", miscTests);
}

}  // namespace tests

}  // namespace Manganese