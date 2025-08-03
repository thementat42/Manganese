/**
 * @file testrunner.h
 * @brief Defines the TestRunner class for running and summarizing unit tests
 */

#ifndef MANGANESE_TESTS_TEST_RUNNER_H
#define MANGANESE_TESTS_TEST_RUNNER_H

#include <global_macros.hpp>

#include <functional>
#include <string>

namespace Manganese {
namespace tests {

class TestRunner {
   private:
    int passed = 0;
    int failed = 0;
    std::string failedTests = "";  // Keep track of failed tests for debugging

   public:
    void runTest(const std::string& testName, std::function<bool()> testFunction);
    void printSummary();
    bool allTestsPassed();
};
}  // namespace tests
}  // namespace Manganese

#endif  // MANGANESE_TESTS_TEST_RUNNER_H