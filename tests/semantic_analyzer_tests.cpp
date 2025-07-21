#include <frontend/ast.h>
#include <frontend/parser.h>
#include <frontend/semantic.h>
#include <global_macros.h>

#include <iostream>

#include "testrunner.h"

namespace Manganese {

namespace tests {

parser::ParsedFile parse(const std::string& source, lexer::Mode mode = lexer::Mode::String) {
    parser::Parser parser(source, mode);
    return parser.parse();
}

bool foo() {
    semantic::SemanticAnalyzer analyzer;
    parser::ParsedFile file = parse("true; \"asdf\"; [1i64, 2, 3, 4, true];");
    analyzer.analyze(file);
    const auto& program = file.program;
    if (program.size() != 3) {
        std::cerr << "Expected 3 statements, got " << program.size() << "\n";
        return false;
    }
    for (size_t i = 0; i < 3; ++i) {
        program[i]->dump(std::cerr);
    }

    return true;
}

void runSemanticAnalysisTests(TestRunner& runner) {
    runner.runTest("Foo", foo);
}

}  // namespace tests

}  // namespace Manganese