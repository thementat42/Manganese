/**
 * @file testrunner.cpp
 * @brief Implementation of the TestRunner class for running and summarizing unit tests.
 *
 * This file provides the implementation for the TestRunner class, which is responsible for:
 * - Running individual tests and reporting their results.
 * - Keeping track of the number of passed and failed tests.
 * - Printing a summary of all test results, including percentages and a list of failed tests.
 */
#include "testrunner.h"

#include <functional>
#include <iomanip>
#include <iostream>
#include <string>

#define TO_2_DP(value) \
    std::fixed << std::setprecision(2) << value

#include <global_macros.h>
#include <io/logging.h>

constexpr inline float percentage(const int part, const int total) {
    return static_cast<float>(part) / static_cast<float>(total) * 100.0f;
}

namespace Manganese {
namespace tests {
void TestRunner::runTest(const std::string& testName, std::function<bool()> testFunction) {
    std::cout << "Running test: " << testName << "...\n";

    bool result = testFunction();
    std::cout << "\nTest " << testName << ": ";

    if (result) {
        std::cout << GREEN << "PASSED" << RESET << '\n';
        ++passed;
    } else {
        std::cout << RED << "FAILED" << RESET << '\n';
        ++failed;
        failedTests += testName + '\n';
    }
}

void TestRunner::printSummary() {
    auto total = passed + failed;
    total = total == 0 ? 1 : total;  // avoid any division by 0 problems
    std::cout << PINK << "\n=== Test Summary ===" << RESET << '\n';
    std::cout << GREEN << "Passed: " << passed << "/" << total << " (" << TO_2_DP(percentage(passed, total)) << "%)" << RESET << '\n';
    std::cout << RED << "Failed: " << failed << "/" << total << " (" << TO_2_DP(percentage(failed, total)) << "%)" << RESET << '\n';
    std::cout << PINK << "Total: " << total << RESET << '\n';
    if (failed > 0) {
        std::cout << PINK << "=== Failed Tests ===" << RESET << '\n';
        std::cout << RED << failedTests << RESET;
    } else {
        std::cout << GREEN << "All tests passed!" << RESET << '\n';
    }
}

bool TestRunner::allTestsPassed() {
    return failed == 0;
}
}  // namespace tests
}  // namespace Manganese
