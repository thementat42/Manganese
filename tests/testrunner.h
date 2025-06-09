#ifndef MANGANESE_TESTS_TEST_RUNNER_H
#define MANGANESE_TESTS_TEST_RUNNER_H

#include <functional>
#include <string>

#ifndef DEBUG
#define DEBUG 1  // Force debug on in tests
#endif  // DEBUG

#include "../src/global_macros.h"

MANGANESE_BEGIN
namespace tests {

// ANSI color codes for terminal output
constexpr const char* GREEN = "\033[32m";
constexpr const char* RED = "\033[31m";
constexpr const char* RESET = "\033[0m";
constexpr const char* PINK = "\033[95m";
constexpr const char* CYAN = "\033[36m";

class TestRunner {
   private:
    int passed = 0;
    int failed = 0;

   public:
    void runTest(const std::string& testName, std::function<bool()> testFunction);
    void printSummary();
    bool allTestsPassed();
};
}  // namespace tests
MANGANESE_END

#endif  // MANGANESE_TESTS_TEST_RUNNER_H