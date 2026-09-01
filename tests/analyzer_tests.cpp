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

    semantic::Analyzer analyzer(parsedFile, arena);
    Result result = analyzer.analyze();

    std::ofstream logFile(logFileName, std::ios::app);
    if (!logFile) {
        std::cerr << "ERROR: Could not open log file for writing.\n";
    } else {
        logFile << "Test: " << testName << "\n";
        logFile << "Expected Result: Semantically " << (expectSuccess ? "Valid" : "Invalid") << "\n";
        logFile << "Actual Result: Semantically " << (result == Result::Success ? "Valid" : "Invalid") << '\n';
        logFile << "Test " << (result == Result::Success ? "Passed" : "Failed") << '\n';
        logFile << "Analyzed " << testName << " AST:\n";

        // Helper lambda to write statement string rep and dump to log
        auto logStatement = [&logFile](const ast::Statement* stmt) {
            if (!stmt) return;
            logFile << "String representation: " << stmt->toString(0) << '\n';
            logFile << "Dumping statement:\n";
            stmt->dump(logFile);  // Node dump includes semanticType when available
            logFile << "---------------------\n";
        };

        if (parsedFile.fileModule) { logStatement(parsedFile.fileModule); }
        for (const auto* importStmt : parsedFile.imports) { logStatement(importStmt); }
        for (const auto& stmt : parsedFile.program) { logStatement(stmt); }

        logFile.close();
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

    // Self-referential aggregate (infinite size layout) must fail
    std::string invalidSource = R"(
        aggregate Node { next: Node; }
    )";

    return analyzeSource(validSource, true, __func__) && 
           analyzeSource(invalidSource, false, __func__);
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

    // break outside of a loop must fail
    std::string invalid = R"(
        func main() {
            break;
        }
    )";

    return analyzeSource(valid, true, __func__) && 
           analyzeSource(invalid, false, __func__);
}

static bool testEnumDeclarationStatement() {
    std::string source = R"(
        enum Colour : int32 { Red, Green, Blue }
        func main() {
            let c = Colour::Red;
        }
    )";
    std::string invalid = R"(
        enum Colour : int32 { Red, Green, Blue }
        enum Colour2 : int32 { Red, Green, Blue }
        func main() {
            let mut c = Colour::Red;
            let c2 = Colour2::Red;
            c = Colour::Green;
            c2 = Colour::Blue;
        }
    )";
    return analyzeSource(source, true, __func__) && 
           analyzeSource(invalid, false, __func__);
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

    // Returning a value in a void/empty-return function
    std::string valueInVoid = R"(
        func doNothing() {
            return 100;
        }
    )";

    // Valid void and non-void functions
    std::string valid = R"(
        func doNothing() { 
            return; 
        }
        func getValue() -> int32 { 
            return 42; 
        }
    )";

    return analyzeSource(missingReturn, false, __func__) && 
           analyzeSource(valueInVoid, false, __func__) && 
           analyzeSource(valid, true, __func__);
}

static bool testIfStatement() {
    // Non-boolean condition must fail
    std::string valid = R"(
        func main() {
            if ("not a bool") {}
        }
    )";

    std::string valid2 = R"(
        func main() {
            if (1 < 2) {} else {}
        }
    )";

    return analyzeSource(valid, true, __func__) && 
           analyzeSource(valid2, true, __func__);
}

static bool testNamespaceStatement() {
    // Valid namespace declarations, reopening, and nested namespaces
    std::string valid = R"(
        namespace Math {
            func add(a: int32, b: int32) -> int32 {
                return a + b;
            }
        }

        # Re-opening the namespace to add more symbols
        namespace Math {
            func sub(a: int32, b: int32) -> int32 {
                return a - b;
            }
        }

        namespace Outer {
            namespace Inner {
                let val: int32 = 42;
            }
        }
    )";

    // Duplicate symbol declaration within the same namespace must fail
    std::string duplicateSymbol = R"(
        namespace Math {
            let x: int32 = 10;
            let x: int32 = 20;
        }
    )";

    return analyzeSource(valid, true, __func__) && 
           analyzeSource(duplicateSymbol, false, __func__);
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

    // Cannot initialize variable with void function result
    std::string voidAssign = R"(
        func foo() {}
        func main() {
            let a = foo();
        }
    )";

    return analyzeSource(mismatch, false, __func__) && 
           analyzeSource(voidAssign, false, __func__);
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

    // Indexing with a non-integer must fail
    std::string invalid = R"(
        func main() {
            let arr = [1, 2, 3];
            let elem = arr["index"];
        }
    )";

    return analyzeSource(valid, true, __func__) && 
           analyzeSource(invalid, false, __func__);
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

    std::string valid = R"(
        func add(a: int32, b: int32) -> int32 { 
            return a + b; 
        }
        func main() {
            let res = add(1, 2);
        }
    )";

    return analyzeSource(voidArg, false, __func__) && 
           analyzeSource(valid, true, __func__);
}

static bool testGenericInstantiationExpression() {
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

    // Dereferencing a non-pointer must fail
    std::string invalid = R"(
        func main() {
            let val: int32 = 10;
            let deref = *val;
        }
    )";

    return analyzeSource(valid, true, __func__) && 
           analyzeSource(invalid, false, __func__);
}

static bool testScopeResolutionExpression() {
    // Valid scope resolution calls and chained lookups
    std::string valid = R"(
        namespace Math {
            func getFortyTwo() -> int32 {
                return 42;
            }
        }

        namespace Outer {
            namespace Inner {
                let number: int32 = 100;
            }
        }

        func main() {
            let a = Math::getFortyTwo();
            let b = Outer::Inner::number;
        }
    )";

    // Accessing a symbol that does not exist in the namespace
    std::string missingMember = R"(
        namespace Math {}
        func main() {
            let x = Math::doesNotExist;
        }
    )";

    // Using a non-namespace/non-module variable as a scope
    std::string invalidScope = R"(
        func main() {
            let notAScope: int32 = 5;
            let x = notAScope::member;
        }
    )";

    return analyzeSource(valid, true, __func__) && 
           analyzeSource(missingMember, false, __func__) && 
           analyzeSource(invalidScope, false, __func__);
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

static bool testGenericScopeResolution() {
    // Valid generic aggregate defined inside a namespace and instantiated with a scoped type
    std::string valid = R"(
        namespace Data {
            aggregate Pair[T] {
                first: T;
                second: T;
            }
            aggregate Point { x: int32; y: int32; }
        }

        func main() {
            let p = Data::Point { x = 1, y = 2 };
            let pair = Data::Pair@[Data::Point] { first = p, second = p };
        }
    )";

    // Attempting to instantiate a generic type using an unbound parameter inside generic scope
    std::string unboundGeneric = R"(
        namespace Data {
            aggregate Container[T] { item: T; }
        }
        func main() {
            let c = Data::Container@[U] { item = 0 };
        }
    )";

    return analyzeSource(valid, true, __func__) && 
           analyzeSource(unboundGeneric, false, __func__);
}

static bool testScopeResolutionMutability() {
    // Valid mutation of a mutable variable in a scope
    std::string valid = R"(
        namespace Globals {
            let mut counter: int32 = 0;
            let maxCount: int32 = 100;
        }

        func main() {
            Globals::counter = Globals::counter + 1;
            Globals::counter++;
        }
    )";

    // Assigning to an immutable variable inside a scope resolution expression must fail
    std::string invalidAssign = R"(
        namespace Globals {
            let maxCount: int32 = 100;
        }

        func main() {
            Globals::maxCount = 200;
        }
    )";

    // Incrementing an immutable scoped variable must fail
    std::string invalidInc = R"(
        namespace Globals {
            let maxCount: int32 = 100;
        }

        func main() {
            Globals::maxCount++;
        }
    )";

    return analyzeSource(valid, true, __func__) && 
           analyzeSource(invalidAssign, false, __func__) && 
           analyzeSource(invalidInc, false, __func__);
}

static bool testDeeplyNestedScopedType() {
    // Arbitrary scoping depth (Outer::Middle::Inner::Target)
    std::string valid = R"(
        namespace Level1 {
            namespace Level2 {
                namespace Level3 {
                    aggregate DeepType { value: int32; }
                }
            }
        }

        func main() {
            let obj: Level1::Level2::Level3::DeepType = Level1::Level2::Level3::DeepType { value = 42 };
        }
    )";

    // Breaking the chain midway through an invalid child scope
    std::string invalidChain = R"(
        namespace Level1 {
            namespace Level2 {}
        }

        func main() {
            let obj: Level1::Level2::NonExistent::DeepType = 0;
        }
    )";

    return analyzeSource(valid, true, __func__) && 
           analyzeSource(invalidChain, false, __func__);
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

    // Coercing mutable pointer to immutable pointer is valid
    std::string valid = R"(
        func main() {
            let mut val: int32 = 10;
            let mutPtr: ptr mut int32 = &val;
            let constPtr: ptr int32 = mutPtr;
        }
    )";

    return analyzeSource(invalid, false, __func__) && 
           analyzeSource(valid, true, __func__);
}

static bool testArrayOfVoidDisallowed() {
    // Array of void elements is invalid
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

static bool testScopedType() {
    // Valid scoped types (aggregates and aliases inside namespaces)
    std::string valid = R"(
        namespace Geometry {
            aggregate Point { x: int32; y: int32; }
            alias Distance = int32;
        }

        namespace Graphics {
            namespace _2D {
                aggregate Canvas { width: int32; height: int32; }
            }
        }

        func main() {
            let p: Geometry::Point = Geometry::Point { x = 0, y = 0 };
            let d: Geometry::Distance = 10;
            let c: Graphics::_2D::Canvas = Graphics::_2D::Canvas { width = 800, height = 600 };
        }
    )";

    // Using a function or non-type symbol as a scoped type
    std::string nonTypeAsType = R"(
        namespace Math {
            func calculate() {}
        }

        func main() {
            let x: Math::calculate = 42;
        }
    )";

    return analyzeSource(valid, true, __func__) && 
           analyzeSource(nonTypeAsType, false, __func__);
}

// Other

static bool testAnalyzeFromFile() {
    std::filesystem::path fullPath = std::filesystem::current_path() / "tests/analyzer_tests.mn";

    mnstl::chunk_allocator file_allocator{};
    parser::Parser parser(fullPath.string(), lexer::Mode::File, file_allocator);
    parser::ParsedFile parsedFile = parser.parse();

    semantic::Analyzer Analyzer(parsedFile, file_allocator);

    std::ofstream logFile(logFileName, std::ios::app);
    Result result = Analyzer.analyze();

    if (!logFile) {
        std::cerr << "ERROR: Could not open log file for writing.\n";
    } else {
        logFile << "Test: Analysis from file\n";
        logFile << "Expected Result: Semantically Valid\n";
        logFile << "Actual Result: Semantically " << (result == Result::Success ? "Valid" : "Invalid") << '\n';
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

    // Assigning through a dereference of an immutable pointer (`ptr int32`) is invalid
    std::string assignThroughConstPtr = R"(
        func main() {
            let target: int32 = 100;
            let constPtr: ptr int32 = &target;
            *constPtr = 200;
        }
    )";

    // Incrementing/decrementing an immutable variable and an rvalue is invalid
    std::string incImmutable = R"(
        func main() {
            let immutableVal: int32 = 10;
            immutableVal++;
            --3;
        }
    )";

    // Taking address of an r-value (or assigning to an r-value) is invalid
    std::string addressOfRValue = R"(
        func main() {
            let ptrVal = &42;
        }
    )";

    return analyzeSource(validSource, true, __func__) && 
           analyzeSource(assignThroughConstPtr, false, __func__) && 
           analyzeSource(incImmutable, false, __func__) && 
           analyzeSource(addressOfRValue, false, __func__);
}

static bool miscTests() {
    std::string code = R"(
        func foo[T](x: T) -> T {
            return x + 3;
        }

        func blah() {
            let a = foo@[int32](10);
            let b = foo@[string]("hello");

        }
    )";
    return analyzeSource(code, false, __func__);
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
    runner.runTest("Namespace Statement analysis", testNamespaceStatement);
    runner.runTest("Scope Resolution Expression analysis", testScopeResolutionExpression);
    runner.runTest("Switch Statement analysis", testSwitchStatement);
    runner.runTest("Variable Declaration Statement analysis", testVariableDeclarationStatement);
    runner.runTest("While Loop Statement analysis", testWhileLoopStatement);
    runner.runTest("Generic Scope Resolution analysis", testGenericScopeResolution);
    runner.runTest("Scope Resolution Mutability analysis", testScopeResolutionMutability);
    runner.runTest("Deeply Nested Scoped Type analysis", testDeeplyNestedScopedType);

    // Expressions
    runner.runTest("Aggregate Instantiation & Literals analysis", testAggregateInstantiationAndLiteralExpression);
    runner.runTest("Array Literals & Indexing analysis", testArrayLiteralAndIndexExpression);
    runner.runTest("Assignment Expression analysis", testAssignmentExpression);
    runner.runTest("Unary & Binary Expressions analysis", testBinaryAndUnaryExpressions);
    runner.runTest("Function Calls & Void Args analysis", testFunctionCallExpression);
    runner.runTest("Generic Expressions analysis", testGenericInstantiationExpression);
    runner.runTest("Member Access Expression analysis", testMemberAccessExpression);
    runner.runTest("Pointer Operators (& and *) analysis", testPrefixPostfixPointerOperators);
    runner.runTest("Sizeof & Alignof Expressions analysis", testSizeofAndAlignofExpression);
    runner.runTest("TypeCast Expression analysis", testTypeCastExpression);

    // Types
    runner.runTest("Pointer Mutability Rules analysis", testPointerTypeMutability);
    runner.runTest("Disallow Array of Void analysis", testArrayOfVoidDisallowed);
    runner.runTest("First-class Function Types analysis", testFunctionTypeAsValue);
    runner.runTest("Scoped Type analysis", testScopedType);

    // Other
    runner.runTest("Analysis from file", testAnalyzeFromFile);
    runner.runTest("Dereference & Immutability analysis", testPointerDereferenceAndMutability);
    runner.runTest("Misc analyzer tests", miscTests);
}

}  // namespace Manganese::tests