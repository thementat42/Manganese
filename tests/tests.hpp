#ifndef MANGANESE_TESTS_TESTS_HPP
#define MANGANESE_TESTS_TESTS_HPP

#include "testrunner.hpp"

namespace Manganese::tests {
void runLexerTests(TestRunner& runner);
void runParserTests(TestRunner& runner);
void runAnalyzerTests(TestRunner& runner);
void runCodeGenerationTests(TestRunner& runner);

}  // namespace Manganese::tests
#endif  // MANGANESE_TESTS_TESTS_HPP