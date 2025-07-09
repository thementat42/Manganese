/**
 * @file tests.h
 * @brief Declaration of the available test functions for the compiler
 */

#ifndef MANGANESE_TESTS_TESTS_H
#define MANGANESE_TESTS_TESTS_H
#include <global_macros.h>

#include "testrunner.h"

namespace Manganese {
namespace tests {
void runLexerTests(TestRunner& runner);
void runParserTests(TestRunner& runner);
void runSemanticAnalysisTests(TestRunner& runner);
void runCodeGenerationTests(TestRunner& runner);

}  // namespace tests
}  // namespace Manganese
#endif  // MANGANESE_TESTS_TESTS_H