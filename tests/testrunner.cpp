#include "testrunner.h"

#include <functional>
#include <iostream>
#include <string>

#include "../src/global_macros.h"
#include "../src/io/include/logging.h"

constexpr inline float percentage(const int part, const int total) {
    return static_cast<float>(part) / static_cast<float>(total) * 100.0f;
}

MANGANESE_BEGIN
namespace tests {
void TestRunner::runTest(const std::string& testName, std::function<bool()> testFunction) {
    std::cout << "Running test: " << testName << "... ";

    bool result = testFunction();

    if (result) {
        std::cout << GREEN << "PASSED" << RESET << '\n';
        ++passed;
    } else {
        std::cout << RED << "FAILED" << RESET << '\n';
        ++failed;
        // TODO: Keep track of which tests failed -- print those
    }
}

void TestRunner::printSummary() {
    auto total = passed + failed;
    total = total == 0 ? 1 : total;  // avoid any division by 0 problems
    std::cout << PINK << "\n=== Test Summary ===" << RESET << '\n';
    std::cout << GREEN << "Passed: " << passed << "/" << total << " (" << percentage(passed, total) << "%)" << RESET << '\n';
    std::cout << RED << "Failed: " << failed << "/" << total << " (" << percentage(failed, total) << "%)" << RESET << '\n';
    std::cout << PINK << "Total: " << total << RESET << '\n';
}

bool TestRunner::allTestsPassed() {
    return failed == 0;
}
}  // namespace tests
MANGANESE_END
