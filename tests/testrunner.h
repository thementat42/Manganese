#ifndef MANGANESE_TESTS_TEST_RUNNER_H
#define MANGANESE_TESTS_TEST_RUNNER_H

#include <functional>
#include <string>

#include "../src/global_macros.h"

namespace manganese {
namespace tests {
class TestRunner {
   private:
    int passed = 0;
    int failed = 0;

    // ANSI color codes for terminal output
    const char* GREEN = "\033[32m";
    const char* RED = "\033[31m";
    const char* RESET = "\033[0m";

   public:
    void runTest(const std::string& testName, std::function<bool()> testFunction);
    void printSummary();
    bool allTestsPassed();
};
}  // namespace tests
}

#endif  // MANGANESE_TESTS_TEST_RUNNER_H