#ifndef MANGANESE_TESTS_TEST_RUNNER_H
#define MANGANESE_TESTS_TEST_RUNNER_H

#include <functional>
#include <string>

#ifndef DEBUG
#define DEBUG 1  // Force debug on in tests
#endif           // DEBUG

#include <global_macros.h>

namespace Manganese {
namespace tests {

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
} // namespace Manganese

#endif  // MANGANESE_TESTS_TEST_RUNNER_H