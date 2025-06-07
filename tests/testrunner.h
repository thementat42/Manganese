#ifndef MANGANESE_TESTS_TEST_RUNNER_H
#define MANGANESE_TESTS_TEST_RUNNER_H

#include <functional>
#include <string>

#include "../src/global_macros.h"

namespace manganese {
namespace tests {

// ANSI color codes for terminal output
constexpr char* GREEN = "\033[32m";
constexpr char* RED = "\033[31m";
constexpr char* RESET = "\033[0m";
constexpr char* PINK = "\033[95m";
constexpr char* CYAN = "\033[36m";

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
}  // namespace manganese

#endif  // MANGANESE_TESTS_TEST_RUNNER_H