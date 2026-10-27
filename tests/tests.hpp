#ifndef MANGANESE_TESTS_TESTS_HPP
#define MANGANESE_TESTS_TESTS_HPP

#include <string>

namespace Manganese::tests {

class TestRunner {
   private:
    int passed = 0;
    int failed = 0;
    std::string passedTests;
    std::string failedTests;  // Keep track of failed tests for debugging

   public:
    void runTest(const std::string& testName, bool (*testFunction)());
    void printSummary() const noexcept;
    bool allTestsPassed() const noexcept { return failed == 0; }
};

void runLexerTests(TestRunner& runner);
void runParserTests(TestRunner& runner);
void runSemanticAnalyzerTests(TestRunner& runner);
void runControlFlowAnalyzerTests(TestRunner& runner);
void runCodeGenerationTests(TestRunner& runner);
void runExecutionTests(TestRunner& runner);

}  // namespace Manganese::tests
#endif  // MANGANESE_TESTS_TESTS_HPP