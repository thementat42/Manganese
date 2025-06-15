/**
 * @file tests.h
 * @brief Declaration of the available test functions for the compiler
 */

#ifndef MANGANESE_TESTS_H
#define MANGANESE_TESTS_H
#include <global_macros.h>

#include "testrunner.h"

MANGANESE_BEGIN
namespace tests {
void runLexerTests(TestRunner& runner);
void runParserTests(TestRunner& runner);
void runSemanticAnalysisTests(TestRunner& runner);
void runCodeGenerationTests(TestRunner& runner);

}  // namespace tests
MANGANESE_END
#endif  // MANGANESE_TESTS_H