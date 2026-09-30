
#ifndef MANGANESE_INCLUDE_FRONTEND_PARSER_MODULE_HANDLING_HPP
#define MANGANESE_INCLUDE_FRONTEND_PARSER_MODULE_HANDLING_HPP 1

#include <frontend/ast.hpp>
#include <frontend/parser/parser_base.hpp>
#include <string_view>
#include <unordered_set>

namespace Manganese::parser {

struct Module {
    std::string_view name;
    std::unordered_set<std::string> dependencies;

    ast::Block fullProgram;
};

std::vector<Module> sortModules(const std::vector<ParsedFile>& parsedFiles);

}  // namespace Manganese::parser

#endif  // MANGANESE_INCLUDE_FRONTEND_PARSER_MODULE_HANDLING_HPP