#include <core.hpp>

#include "tests/tests.hpp"

int main(int argc, const char** argv) {
    const auto testResult = Manganese::tests::runTestSuite(argc, argv);
    return testResult == Manganese::Result::Success ? 0 : 1;
}
