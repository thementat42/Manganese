
#ifndef MANGANESE_INCLUDE_FRONTEND_PARSER_MODULE_HANDLING_HPP
#define MANGANESE_INCLUDE_FRONTEND_PARSER_MODULE_HANDLING_HPP 1

#include <frontend/ast.hpp>
#include <frontend/parser/parser_base.hpp>
#include <string>
#include <unordered_set>
#include <utils/chunk_allocator.hpp>
#include <utils/string_interner.hpp>

namespace Manganese::parser {

struct Module {
    std::string name;
    std::unordered_set<std::string> dependencies;
    std::vector<ast::Statement*> statementsBuffer;  // Temporary vector for building the block
    ast::Block fullProgram;
};

std::vector<Module> sortModules(const std::vector<ParsedFile>& parsedFiles, const utils::StringInterner& interner);

std::vector<ParsedFile> parseInParallel(const std::vector<std::string>& filenames, utils::chunk_allocator& globalArena,
                                        utils::StringInterner& interner);

}  // namespace Manganese::parser

#endif  // MANGANESE_INCLUDE_FRONTEND_PARSER_MODULE_HANDLING_HPP