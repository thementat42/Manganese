#include "testrunner.hpp"

#include <core.hpp>
#include <io/logging.hpp>
#include <iostream>
#include <string>

namespace Manganese::tests {
void TestRunner::runTest(const std::string& testName, bool (*testFunction)()) {
    std::cout << "Running test: " << testName << "...\n";

    bool result = testFunction();
    std::cout << (result ? GREEN : RED) << std::format("Test '{}' {}", testName, (result ? "PASSED" : "FAILED"))
              << RESET << "\n";

    if (result) {
        ++passed;
        passedTests += testName + '\n';
    } else {
        ++failed;
        failedTests += testName + '\n';
    }
}

void TestRunner::printSummary() const noexcept {
    int total = passed + failed;
    total = total == 0 ? 1 : total;  // avoid any division by 0 problems
    std::cout << PINK << "\nTest Results:" << RESET << '\n';
    const float percentPassed = static_cast<float>(passed) / static_cast<float>(total) * 100.0f;
    const float percentFailed = static_cast<float>(failed) / static_cast<float>(total) * 100.0f;

    if (failed > 0) {
        std::cout << GREEN << std::format("Tests Passed ({}):\n", passed) << passedTests << "\n";
        std::cout << RED << std::format("Tests Failed ({}):\n", failed) << failedTests << RESET << "\n";
    } else {
        std::cout << GREEN << "All tests passed!" << RESET << '\n';
    }

    std::cout << "\n----------\n" << PINK << "Test Summary: " << RESET << '\n';
    std::cout << GREEN << std::format("Passed: {}/{} ({:.2f}%)\n", passed, total, percentPassed) << RESET;
    std::cout << RED << std::format("Failed: {}/{} ({:.2f}%)\n", failed, total, percentFailed) << RESET;
    std::cout << PINK << "Total: " << total << RESET << '\n';
}
}  // namespace Manganese::tests
