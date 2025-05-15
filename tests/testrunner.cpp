#include "testrunner.h"

#include <functional>
#include <iostream>
#include <string>

void TestRunner::runTest(const std::string& testName, std::function<bool()> testFunction) {
    std::cout << "Running test: " << testName << "... ";

    bool result = testFunction();

    if (result) {
        std::cout << GREEN << "PASSED" << RESET << '\n';
        ++passed;
    } else {
        std::cout << RED << "FAILED" << RESET << '\n';
        ++failed;
    }
}

void TestRunner::printSummary() {
    std::cout << "\n=== Test Summary ===" << '\n';
    std::cout << GREEN << "Passed: " << passed << RESET << '\n';
    std::cout << RED << "Failed: " << failed << RESET << '\n';
    std::cout << "Total: " << (passed + failed) << '\n';
}

bool TestRunner::allTestsPassed() {
    return failed == 0;
}
