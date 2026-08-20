#include <cassert>
#include <core.hpp>
#include <filesystem>
#include <frontend/ast.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic/analyzer.hpp>
#include <frontend/semantic/type_context.hpp>
#include <fstream>
#include <iostream>
#include <string>

#include "testrunner.hpp"

namespace Manganese::tests {

constexpr static const char* logFileName = "logs/analyzer_tests.log";
static mnstl::chunk_allocator arena;

// Helper: Parses and runs full semantic analysis on source code
static bool analyzeSource(const std::string& source, bool expectSuccess, std::string_view testName) {
    parser::Parser parser(source, lexer::Mode::String, arena);
    parser::ParsedFile parsedFile = parser.parse();

    semantic::analyzer analyzer(parsedFile, arena);
    Result result = analyzer.analyze();

    std::ofstream logFile(logFileName, std::ios::app);
    if (!logFile) {
        std::cerr << "ERROR: Could not open log file for writing.\n";
    } else {
        logFile << "Test: " << testName << "\n";
        logFile << "Expected Result: " << (expectSuccess ? "Success" : "Failure") << "\n";
        logFile << "Actual Result: " << (result == Result::Success ? "Success" : "Failure") << '\n';
        logFile << "Analyzed " << testName << "AST:\n";
        for (const auto& stmt : parsedFile.program) {
            logFile << "String representation: " << stmt->toString(0) << '\n';
            logFile << "Dumping statement:\n";
            stmt->dump(logFile);  // Node dump includes semanticType when available
            logFile << "---------------------\n";

            logFile.close();
        }
    }

    return expectSuccess ? result == Result::Success : result == Result::Failure;
}

// Statement Tests

static bool testAggregateDeclarationStatement() {
    // Basic aggregate declaration and recursive layout prevention
    std::string validSource = R"(
        aggregate Point { x: int32; y: int32; }
        aggregate Line { start: Point; end: Point; }
    )";
    if (!analyzeSource(validSource, true, __func__)) { return false; }

    // Self-referential aggregate (infinite size layout) must fail
    std::string invalidSource = R"(
        aggregate Node { next: Node; }
    )";
    return analyzeSource(invalidSource, false, __func__);
}

static bool testAliasStatement() {
    std::string source = R"(
        alias MyInt = int32;
        func main() {
            let x: MyInt = 42;
        }
    )";
    return analyzeSource(source, true, __func__);
}

static bool testBreakAndContinueStatement() {
    std::string valid = R"(
        func main() {
            while (true) {
                if (false) { 
                    break; 
                }
                continue;
            }
        }
    )";
    if (!analyzeSource(valid, true, __func__)) { return false; }

    // break outside of a loop must fail
    std::string invalid = R"(
        func main() {
            break;
        }
    )";
    return analyzeSource(invalid, false, __func__);
}

static bool testEnumDeclarationStatement() {
    std::string source = R"(
        enum Color : int32 { Red, Green, Blue }
    )";
    return analyzeSource(source, true, __func__);
}

static bool testForLoopStatement() {
    std::string source = R"(
        func main() {
            for (let mut i: int32 = 0; i < 10; i = i + 1) {}
        }
    )";
    return analyzeSource(source, true, __func__);
}

static bool testFunctionDeclarationAndReturnStatement() {
    // Missing return expression in typed function
    std::string missingReturn = R"(
        func getValue() -> int32 {
            return;
        }
    )";
    if (!analyzeSource(missingReturn, false, __func__)) { return false; }

    // Returning a value in a void/empty-return function
    std::string valueInVoid = R"(
        func doNothing() {
            return 100;
        }
    )";
    if (!analyzeSource(valueInVoid, false, __func__)) { return false; }

    // Valid void and non-void functions
    std::string valid = R"(
        func doNothing() { 
            return; 
        }
        func getValue() -> int32 { 
            return 42; 
        }
    )";
    return analyzeSource(valid, true, __func__);
}

static bool testIfStatement() {
    // Non-boolean condition must fail
    std::string valid = R"(
        func main() {
            if ("not a bool") {}
        }
    )";
    if (!analyzeSource(valid, true, __func__)) { return true; }

    std::string valid2 = R"(
        func main() {
            if (1 < 2) {} else {}
        }
    )";
    return analyzeSource(valid2, true, __func__);
}

[[maybe_unused]] static bool testNamespaceStatement() {
    // TODO
    return true;
}

static bool testSwitchStatement() {
    std::string source = R"(
        func main() {
            let x: int32 = 1;
            switch (x) {
                case 1: {}
                default: {}
            }
        }
    )";
    return analyzeSource(source, true, __func__);
}

static bool testVariableDeclarationStatement() {
    // Type mismatch on initialization
    std::string mismatch = R"(
        func main() {
            let a: int32 = "hello";
        }
    )";
    if (!analyzeSource(mismatch, false, __func__)) { return false; }

    // Cannot initialize variable with void function result
    std::string voidAssign = R"(
        func foo() {}
        func main() {
            let a = foo();
        }
    )";
    return analyzeSource(voidAssign, false, __func__);
}

static bool testWhileLoopStatement() {
    std::string source = R"(
        func main() {
            while (true) {}
            do {} while (true);
        }
    )";
    return analyzeSource(source, true, __func__);
}

// Expression Tests

static bool testAggregateInstantiationAndLiteralExpression() {
    std::string source = R"(
        aggregate Point { x: int32; y: int32; }
        func main() {
            let p = Point { x = 10, y = 20 };
        }
    )";
    return analyzeSource(source, true, __func__);
}

static bool testArrayLiteralAndIndexExpression() {
    std::string valid = R"(
        func main() {
            let arr = [1, 2, 3];
            let elem = arr[0];
        }
    )";
    if (!analyzeSource(valid, true, __func__)) { return false; }

    // Indexing with a non-integer must fail
    std::string invalid = R"(
        func main() {
            let arr = [1, 2, 3];
            let elem = arr["index"];
        }
    )";
    return analyzeSource(invalid, false, __func__);
}

static bool testAssignmentExpression() {
    std::string source = R"(
        func main() {
            let mut x: int32 = 5;
            x = 10;
        }
    )";
    return analyzeSource(source, true, __func__);
}

static bool testBinaryAndUnaryExpressions() {
    std::string source = R"(
        func main() {
            let a = 10 + 20 * 30;
            let b = !false;
            let c = -a;
        }
    )";
    return analyzeSource(source, true, __func__);
}

static bool testFunctionCallExpression() {
    // Passing void expression as an argument must fail
    std::string voidArg = R"(
        func takeInt(x: int32) {}
        func getNothing() {}
        func main() {
            takeInt(getNothing());
        }
    )";
    if (!analyzeSource(voidArg, false, __func__)) { return false; }

    std::string valid = R"(
        func add(a: int32, b: int32) -> int32 { 
            return a + b; 
        }
        func main() {
            let res = add(1, 2);
        }
    )";
    return analyzeSource(valid, true, __func__);
}

static bool testGenericExpression() {
    std::string source = R"(
        aggregate Container[T]{ value: T; }
        func main() {
            let c = Container@[int32] { value = 42 };
        }
    )";
    return analyzeSource(source, true, __func__);
}

static bool testMemberAccessExpression() {
    std::string source = R"(
        aggregate Point { x: int32; y: int32; }
        func main() {
            let p = Point { x = 1, y = 2 };
            let xVal = p.x;
        }
    )";
    return analyzeSource(source, true, __func__);
}

static bool testPrefixPostfixPointerOperators() {
    std::string valid = R"(
        func main() {
            let mut val: int32 = 10;
            let pointer: ptr int32 = &val;
            let deref = *pointer;
            val++;
        }
    )";
    if (!analyzeSource(valid, true, __func__)) { return false; }

    // Dereferencing a non-pointer must fail
    std::string invalid = R"(
        func main() {
            let val: int32 = 10;
            let deref = *val;
        }
    )";
    return analyzeSource(invalid, false, __func__);
}

[[maybe_unused]] static bool testScopeResolutionExpression() {
    // TODO
    return true;
}

static bool testSizeofAndAlignofExpression() {
    std::string source = R"(
        func main() {
            let s = sizeof(int32);
            let a = alignof(int64);
        }
    )";
    return analyzeSource(source, true, __func__);
}

static bool testTypeCastExpression() {
    std::string source = R"(
        func main() {
            let x: int32 = 10;
            let y = x as int64;
        }
    )";
    return analyzeSource(source, true, __func__);
}

// Type Tests

static bool testPointerTypeMutability() {
    std::string invalid = R"(
        func main() {
            let val: int32 = 10;
            let constPtr: ptr int32 = &val;
            let mutPtr: ptr mut int32 = constPtr;
        }
    )";
    if (!analyzeSource(invalid, false, __func__)) { return false; }

    // Coercing mutable pointer to immutable pointer is valid
    std::string valid = R"(
        func main() {
            let mut val: int32 = 10;
            let mutPtr: ptr mut int32 = &val;
            let constPtr: ptr int32 = mutPtr;
        }
    )";
    return analyzeSource(valid, true, __func__);
}

static bool testArrayOfVoidDisallowed() {
    // Array of void elements must fail
    std::string source = R"(
        func getVoid() {}
        func main() {
            let arr = [getVoid(), getVoid()];
        }
    )";
    return analyzeSource(source, false, __func__);
}

static bool testFunctionTypeAsValue() {
    std::string source = R"(
        func add(a: int32, b: int32) -> int32 { 
            return a + b; 
        }
        func main() {
            let fnPtr: func(int32, int32) -> int32 = add;
            fnPtr(1,2);
            let x = [fnPtr, add];
            x[0](1,1);
        }
    )";
    return analyzeSource(source, true, __func__);
}

[[maybe_unused]] static bool testScopedType() {
    // TODO
    return true;
}

// Other

static bool testAnalyzeFromFile() {
    std::filesystem::path fullPath = std::filesystem::current_path() / "tests/analyzer_tests.mn";

    mnstl::chunk_allocator file_allocator{};
    parser::Parser parser(fullPath.string(), lexer::Mode::File, file_allocator);
    parser::ParsedFile parsedFile = parser.parse();

    semantic::analyzer analyzer(parsedFile, file_allocator);

    std::ofstream logFile(logFileName, std::ios::app);
    Result result = analyzer.analyze();

    if (!logFile) {
        std::cerr << "ERROR: Could not open log file for writing.\n";
    } else {
        logFile << "Test: Analysis from file\n";
        logFile << "Expected Result: Success\n";
        logFile << "Actual Result: " << (result == Result::Success ? "Success" : "Failure") << '\n';
        logFile << "Analyzed File AST:\n";
        for (const auto& stmt : parsedFile.program) {
            logFile << "String representation: " << stmt->toString(0) << '\n';
            logFile << "Dumping statement:\n";
            stmt->dump(logFile);  // Node dump includes semanticType when available
            logFile << "---------------------\n";
            logFile.close();
        }
    }

    return result == Result::Success;
}

static bool testPointerDereferenceAndMutability() {
    std::string validSource = R"(
        func main() {
            let mut target: int32 = 100;
            let immutableTarget: int32 = 50;

            # Address-of on mutable variable produces `ptr mut int32`
            let mut mutPtr: ptr mut int32 = &target;

            # Address-of on immutable variable produces `ptr int32`
            let constPtr: ptr int32 = &immutableTarget;

            # Dereference assignment to `ptr mut`
            *mutPtr = 200;

            # Pointer arithmetic
            mutPtr++;
            --mutPtr;
        }
    )";
    if (!analyzeSource(validSource, true, __func__)) { return false; }

    // Assigning through a dereference of an immutable pointer (`ptr int32`) is invalid
    std::string assignThroughConstPtr = R"(
        func main() {
            let target: int32 = 100;
            let constPtr: ptr int32 = &target;
            *constPtr = 200;
        }
    )";
    if (!analyzeSource(assignThroughConstPtr, false, __func__)) { return false; }

    // Incrementing/decrementing an immutable variable and an rvalue is invalid
    std::string incImmutable = R"(
        func main() {
            let immutableVal: int32 = 10;
            immutableVal++;
            --3;
        }
    )";
    if (!analyzeSource(incImmutable, false, __func__)) { return false; }

    // Taking address of an r-value (or assigning to an r-value) is invalid
    std::string addressOfRValue = R"(
        func main() {
            let ptrVal = &42;
        }
    )";
    return analyzeSource(addressOfRValue, false, __func__);
}

void runAnalyzerTests(TestRunner& runner) {
    std::ofstream logFile(logFileName, std::ios::trunc);
    logFile.close();
    // Statements
    runner.runTest("Aggregate Declaration Statement analysis", testAggregateDeclarationStatement);
    runner.runTest("Alias Statement analysis", testAliasStatement);
    runner.runTest("Break and Continue Statements analysis", testBreakAndContinueStatement);
    runner.runTest("Enum Declaration Statement analysis", testEnumDeclarationStatement);
    runner.runTest("For Loop Statement analysis", testForLoopStatement);
    runner.runTest("Function Declaration & Returns analysis", testFunctionDeclarationAndReturnStatement);
    runner.runTest("If Statement Conditions analysis", testIfStatement);
    runner.runTest("Switch Statement analysis", testSwitchStatement);
    runner.runTest("Variable Declaration Statement analysis", testVariableDeclarationStatement);
    runner.runTest("While Loop Statement analysis", testWhileLoopStatement);

    // Expressions
    runner.runTest("Aggregate Instantiation & Literals analysis", testAggregateInstantiationAndLiteralExpression);
    runner.runTest("Array Literals & Indexing analysis", testArrayLiteralAndIndexExpression);
    runner.runTest("Assignment Expression analysis", testAssignmentExpression);
    runner.runTest("Unary & Binary Expressions analysis", testBinaryAndUnaryExpressions);
    runner.runTest("Function Calls & Void Args analysis", testFunctionCallExpression);
    runner.runTest("Generic Expressions analysis", testGenericExpression);
    runner.runTest("Member Access Expression analysis", testMemberAccessExpression);
    runner.runTest("Pointer Operators (& and *) analysis", testPrefixPostfixPointerOperators);
    runner.runTest("Sizeof & Alignof Expressions analysis", testSizeofAndAlignofExpression);
    runner.runTest("TypeCast Expression analysis", testTypeCastExpression);

    // Types
    runner.runTest("Pointer Mutability Rules analysis", testPointerTypeMutability);
    runner.runTest("Disallow Array of Void analysis", testArrayOfVoidDisallowed);
    runner.runTest("First-class Function Types analysis", testFunctionTypeAsValue);

    // Other
    runner.runTest("Analysis from file", testAnalyzeFromFile);
    runner.runTest("Dereference & Immutability", testPointerDereferenceAndMutability);
}

}  // namespace Manganese::tests