#include <utils/result.hpp>

#include "tests/tests.hpp"

int main(int argc, const char** argv) {
    const auto testResult = Manganese::tests::runTestSuite(argc, argv);
    return static_cast<int>(testResult == Manganese::Result::Success);
}
