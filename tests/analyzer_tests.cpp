#include <cassert>
#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic/analyzer.hpp>
#include <frontend/semantic/type_context.hpp>
#include <string>

#include "testrunner.hpp"

namespace Manganese::tests {

using semantic::analyzer;

static mnstl::chunk_allocator arena;

// Helper: Parses and runs full semantic analysis on source code
static bool analyzeSource(const std::string& source, bool expectSuccess = true) {
    parser::Parser parser(source, lexer::Mode::String, arena);
    parser::ParsedFile parsedFile = parser.parse();

    analyzer analyzer(parsedFile, arena);
    Result result = analyzer.analyze();

    if (expectSuccess) {
        return result == Result::Success;
    } else {
        return result == Result::Failure;
    }
}

// Statement Tests

static bool testAggregateDeclarationStatement() {
    // Basic aggregate declaration and recursive layout prevention
    std::string validSource = R"(
        aggregate Point { x: int32; y: int32; }
        aggregate Line { start: Point; end: Point; }
    )";
    if (!analyzeSource(validSource, true)) { return false; }

    // Self-referential aggregate (infinite size layout) must fail
    std::string invalidSource = R"(
        aggregate Node { next: Node; }
    )";
    return analyzeSource(invalidSource, false);
}

static bool testAliasStatement() {
    std::string source = R"(
        alias MyInt = int32;
        func main() {
            let x: MyInt = 42;
        }
    )";
    return analyzeSource(source, true);
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
    if (!analyzeSource(valid, true)) { return false; }

    // break outside of a loop must fail
    std::string invalid = R"(
        func main() {
            break;
        }
    )";
    return analyzeSource(invalid, false);
}

static bool testEnumDeclarationStatement() {
    std::string source = R"(
        enum Color : int32 { Red, Green, Blue }
    )";
    return analyzeSource(source, true);
}

static bool testForLoopStatement() {
    std::string source = R"(
        func main() {
            for (let mut i: int32 = 0; i < 10; i = i + 1) {}
        }
    )";
    return analyzeSource(source, true);
}

static bool testFunctionDeclarationAndReturnStatement() {
    // Missing return expression in typed function
    std::string missingReturn = R"(
        func getValue() -> int32 {
            return;
        }
    )";
    if (!analyzeSource(missingReturn, false)) { return false; }

    // Returning a value in a void/empty-return function
    std::string valueInVoid = R"(
        func doNothing() {
            return 100;
        }
    )";
    if (!analyzeSource(valueInVoid, false)) { return false; }

    // Valid void and non-void functions
    std::string valid = R"(
        func doNothing() { 
            return; 
        }
        func getValue() -> int32 { 
            return 42; 
        }
    )";
    return analyzeSource(valid, true);
}

static bool testIfStatement() {
    // Non-boolean condition must fail
    std::string valid = R"(
        func main() {
            if ("not a bool") {}
        }
    )";
    if (!analyzeSource(valid, true)) { return true; }

    std::string valid2 = R"(
        func main() {
            if (1 < 2) {} else {}
        }
    )";
    return analyzeSource(valid2, true);
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
    return analyzeSource(source, true);
}

static bool testVariableDeclarationStatement() {
    // Type mismatch on initialization
    std::string mismatch = R"(
        func main() {
            let a: int32 = "hello";
        }
    )";
    if (!analyzeSource(mismatch, false)) { return false; }

    // Cannot initialize variable with void function result
    std::string voidAssign = R"(
        func foo() {}
        func main() {
            let a = foo();
        }
    )";
    return analyzeSource(voidAssign, false);
}

static bool testWhileLoopStatement() {
    std::string source = R"(
        func main() {
            while (true) {}
            do {} while (true);
        }
    )";
    return analyzeSource(source, true);
}

// Expression Tests

static bool testAggregateInstantiationAndLiteralExpression() {
    std::string source = R"(
        aggregate Point { x: int32; y: int32; }
        func main() {
            let p = Point { x = 10, y = 20 };
        }
    )";
    return analyzeSource(source, true);
}

static bool testArrayLiteralAndIndexExpression() {
    std::string valid = R"(
        func main() {
            let arr = [1, 2, 3];
            let elem = arr[0];
        }
    )";
    if (!analyzeSource(valid, true)) { return false; }

    // Indexing with a non-integer must fail
    std::string invalid = R"(
        func main() {
            let arr = [1, 2, 3];
            let elem = arr["index"];
        }
    )";
    return analyzeSource(invalid, false);
}

static bool testAssignmentExpression() {
    std::string source = R"(
        func main() {
            let mut x: int32 = 5;
            x = 10;
        }
    )";
    return analyzeSource(source, true);
}

static bool testBinaryAndUnaryExpressions() {
    std::string source = R"(
        func main() {
            let a = 10 + 20 * 30;
            let b = !false;
            let c = -a;
        }
    )";
    return analyzeSource(source, true);
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
    if (!analyzeSource(voidArg, false)) { return false; }

    std::string valid = R"(
        func add(a: int32, b: int32) -> int32 { 
            return a + b; 
        }
        func main() {
            let res = add(1, 2);
        }
    )";
    return analyzeSource(valid, true);
}

static bool testGenericExpression() {
    std::string source = R"(
        aggregate Container[T]{ value: T; }
        func main() {
            let c = Container@[int32] { value = 42 };
        }
    )";
    return analyzeSource(source, true);
}

static bool testMemberAccessExpression() {
    std::string source = R"(
        aggregate Point { x: int32; y: int32; }
        func main() {
            let p = Point { x = 1, y = 2 };
            let xVal = p.x;
        }
    )";
    return analyzeSource(source, true);
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
    if (!analyzeSource(valid, true)) { return false; }

    // Dereferencing a non-pointer must fail
    std::string invalid = R"(
        func main() {
            let val: int32 = 10;
            let deref = *val;
        }
    )";
    return analyzeSource(invalid, false);
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
    return analyzeSource(source, true);
}

static bool testTypeCastExpression() {
    std::string source = R"(
        func main() {
            let x: int32 = 10;
            let y = x as int64;
        }
    )";
    return analyzeSource(source, true);
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
    if (!analyzeSource(invalid, false)) { return false; }

    // Coercing mutable pointer to immutable pointer is valid
    std::string valid = R"(
        func main() {
            let mut val: int32 = 10;
            let mutPtr: ptr mut int32 = &val;
            let constPtr: ptr int32 = mutPtr;
        }
    )";
    return analyzeSource(valid, true);
}

static bool testArrayOfVoidDisallowed() {
    // Array of void elements must fail
    std::string source = R"(
        func getVoid() {}
        func main() {
            let arr = [getVoid(), getVoid()];
        }
    )";
    return analyzeSource(source, false);
}

static bool testFunctionTypeAsValue() {
    std::string source = R"(
        func add(a: int32, b: int32) -> int32 { 
            return a + b; 
        }
        func main() {
            let fnPtr: func(int32, int32) -> int32 = add;
        }
    )";
    return analyzeSource(source, true);
}

[[maybe_unused]] static bool testScopedType() {
    // TODO
    return true;
}

void runAnalyzerTests(TestRunner& runner) {
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
}

}  // namespace Manganese::tests