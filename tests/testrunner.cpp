#include "testrunner.h"

#include <functional>
#include <iostream>
#include <string>

#include "../src/global_macros.h"

namespace manganese {
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
    std::cout << GREEN << "Passed: " << passed << "/" << total << " (" << (passed/total)*100 << "%)" << RESET << '\n';
    std::cout << RED << "Failed: " << failed << "/" << total << " (" << (failed/total)*100 << "%)" << RESET << '\n';
    std::cout << PINK << "Total: " << total << RESET << '\n';
}

bool TestRunner::allTestsPassed() {
    return failed == 0;
}
}  // namespace tests
}  // namespace manganese
