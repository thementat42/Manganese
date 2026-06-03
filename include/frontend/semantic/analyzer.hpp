#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP

#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic/primitives.hpp>
#include <frontend/semantic/symbol_table.hpp>

namespace Manganese {

namespace semantic {
using _analyzer_base_t = ast::Visitor<Result, Result, Result>;

class analyzer final : public _analyzer_base_t {
    // note: with `final`, the compiler can more intelligently detect when analyzer is abstract
    // (i.e., a particular visit() override hasn't been declared), instead of failing at an analyzer instantiation
   private:
    SymbolTable symbolTable;
    parser::ParsedFile& parsedFile;
    const primitives& primitiveTypes;

    // Cached primitive types

   public:
    analyzer(parser::ParsedFile& file, const primitives& primitiveTypesReference) : symbolTable(), parsedFile(file), primitiveTypes(primitiveTypesReference) {}

    Result analyze() {
        collectTypes();
        collectGlobals();
        collectAndSpecializeGenerics();
        Result isSemanticallyValid = checkStatements();
        return isSemanticallyValid;
    }
    ~analyzer() override = default;

   private:
    inline void collectTypes() {  // first pass -- collect all user-defined types
        for (const auto& stmt : parsedFile.program) { _collectTypesInStatement(stmt); }
    }

    bool areTypesCompatible(ast::Type*, ast::Type*) { return false; }
    void _collectTypesInStatement(ast::Statement*);
    void _collectTypesInStatementBody(ast::Statement*);
    void collectGlobals();  // second pass -- collect publicly available symbols for modules
    void collectAndSpecializeGenerics() {
        // third pass -- look at specific generic instantiations and specialize them (e.g.
        // foo@[int] -> create a specialization of foo w/ int)
        // does nothing for now
    }
    inline Result checkStatements() {  // semantic analysis pass (this can also check the generic specializations)
        Result programIsSemanticallyValid = Result::Success;
        for (auto& stmt : parsedFile.program) {
            // Note: don't do a shortcut with && (e.g. valid && visit) since `&&` short circuits but visit has side
            // effects. thus, if the program is invalid visit will never run (since false && anything is false so the
            // right hand side isn't evaluated) which means that once one visit is invalid, all subsequent visits will
            // never happen (limiting the compiler to 1 error at a time)
            if (this->visit(stmt) == Result::Failure) { programIsSemanticallyValid = Result::Failure; }
        }
        return programIsSemanticallyValid;
    }

   protected:
    // overrides for visitor functions
    using _analyzer_base_t::visit;

#define STMT(name, str) stmtvisit_t visit(ast::name*) override;
#define EXPR(name, str) exprvisit_t visit(ast::name*) override;
#define TYPE(name, str) typevisit_t visit(ast::name*) override;

#include <frontend/ast/ast.def>

#undef STMT
#undef EXPR
#undef TYPE
};

}  // namespace semantic

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP