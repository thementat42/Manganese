/**
 * @file testrunner.h
 * @brief Defines the TestRunner class for running and summarizing unit tests
 */

#ifndef MANGANESE_TESTS_TEST_RUNNER_HPP
#define MANGANESE_TESTS_TEST_RUNNER_HPP

#include <global_macros.hpp>

#include <string>

namespace Manganese {
namespace tests {

class TestRunner {
   private:
    int passed = 0;
    int failed = 0;
    std::string failedTests = "";  // Keep track of failed tests for debugging

   public:
    void runTest(const std::string& testName, bool (*testFunction)());
    void printSummary() noexcept;
    constexpr bool allTestsPassed() const noexcept {return failed == 0;}
};
}  // namespace tests
}  // namespace Manganese

#endif  // MANGANESE_TESTS_TEST_RUNNER_HPP