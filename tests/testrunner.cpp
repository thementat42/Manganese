#include "testrunner.hpp"

#include <core.hpp>
#include <io/logging.hpp>
#include <iostream>
#include <string>

constexpr float percentage(const int part, const int total) {
    return static_cast<float>(part) / static_cast<float>(total) * 100.0f;
}

namespace Manganese {
namespace tests {
void TestRunner::runTest(const std::string& testName, bool (*testFunction)()) {
    std::cout << "Running test: " << testName << "...\n";

    bool result = testFunction();
    std::cout << (result ? GREEN : RED) << std::format("Test '{}' {}", testName, (result ? "PASSED" : "FAILED"))
              << RESET << "\n";

    if (result) {
        ++passed;
    } else {
        ++failed;
        failedTests += testName + '\n';
    }
}

void TestRunner::printSummary() noexcept {
    int total = passed + failed;
    total = total == 0 ? 1 : total;  // avoid any division by 0 problems
    std::cout << PINK << "\nTest Summary" << RESET << '\n';
    std::cout << GREEN << std::format("Passed: {}/{} ({:.2f}%)\n", passed, total, percentage(passed, total)) << RESET;
    std::cout << RED << std::format("Failed: {}/{} ({:.2f}%)\n", failed, total, percentage(failed, total)) << RESET;
    std::cout << PINK << "Total: " << total << RESET << '\n';
    if (failed > 0) {
        std::cout << PINK << "Failed Tests" << RESET << '\n';
        std::cout << RED << failedTests << RESET;
    } else {
        std::cout << GREEN << "All tests passed!" << RESET << '\n';
    }
}
}  // namespace tests
}  // namespace Manganese
