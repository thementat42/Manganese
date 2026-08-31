#include "testrunner.hpp"

#include <format>
#include <io/logging.hpp>
#include <iostream>
#include <string>

namespace Manganese::tests {
void TestRunner::runTest(const std::string& testName, bool (*testFunction)()) {
    std::cout << "Running test: " << testName << "...\n";

    const bool result = testFunction();
    std::cout << (result ? ansi::GREEN : ansi::RED)
              << std::format("Test '{}' {}", testName, (result ? "PASSED" : "FAILED")) << ansi::RESET << "\n";

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
    std::cout << ansi::PINK << "\nTest Results:" << ansi::RESET << '\n';

    std::cout << ansi::GREEN << std::format("Tests Passed ({}):\n", passed) << passedTests << "\n";
    std::cout << ansi::RED << std::format("Tests Failed ({}):\n", failed) << failedTests << ansi::RESET << "\n";

    const float percentPassed = static_cast<float>(passed) / static_cast<float>(total) * 100.0F;
    const float percentFailed = static_cast<float>(failed) / static_cast<float>(total) * 100.0F;

    std::cout << "\n----------\n" << ansi::PINK << "Test Summary: " << ansi::RESET << '\n';
    std::cout << ansi::GREEN << std::format("Passed: {}/{} ({:.2f}%)\n", passed, total, percentPassed) << ansi::RESET;
    std::cout << ansi::RED << std::format("Failed: {}/{} ({:.2f}%)\n", failed, total, percentFailed) << ansi::RESET;
    std::cout << ansi::PINK << "Total: " << total << ansi::RESET << '\n';
    if (failed == 0) { std::cout << ansi::GREEN << "All tests passed!" << ansi::RESET << '\n'; }
}
}  // namespace Manganese::tests
