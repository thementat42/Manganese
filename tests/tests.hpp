#ifndef MANGANESE_TESTS_TESTS_HPP
#define MANGANESE_TESTS_TESTS_HPP

#include <string>
#include <utils/result.hpp>

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

    void runLexerTests();
    void runParserTests();
    void runSemanticAnalyzerTests();
    void runControlFlowAnalyzerTests();
    void runCodeGenerationTests();
    void runExecutionTests();
    void runObjectCodeTests();
};
Result runTestSuite(int argc, const char** argv);

}  // namespace Manganese::tests
#endif  // MANGANESE_TESTS_TESTS_HPP