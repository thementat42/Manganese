#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_ANALYZER_HPP

#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic/symbol_table.hpp>
#include <global_macros.hpp>

namespace Manganese {

namespace semantic {
using _analyzer_base_t = ast::Visitor<bool, bool, bool>;

class analyzer final : public _analyzer_base_t {
    // note: with `final`, the compiler can more intelligently detect when analyzer is abstract
    // (i.e., a particular visit() override hasn't been declared), instead of failing at an analyzer instantiation
   private:
    SymbolTable table;
    parser::ParsedFile& parsed;

   public:
    analyzer(parser::ParsedFile& file) : table(), parsed(file) {}
    bool analyze() {
        collectTypes();
        collectGlobals();
        collectAndSpecializeGenerics();
        bool isSemanticallyValid = checkStatements();
        return isSemanticallyValid;
    }
    ~analyzer() override = default;

   private:
    inline void collectTypes() {  // first pass -- collect all user-defined types
        for (const auto& stmt : parsed.program) { _collectTypesInStatement(stmt); }
    }
    void _collectTypesInStatement(ast::Statement*);
    void _collectTypesInStatementBody(ast::Statement*);
    void collectGlobals();  // second pass -- collect publicly available symbols for modules
    void collectAndSpecializeGenerics() {
        // third pass -- look at specific generic instantiations and specialize them (e.g.
        // foo@[int] -> create a specialization of foo w/ int)
        // does nothing for now
    }
    inline bool checkStatements() {  // semantic analysis pass (this can also check the generic specializations)
        bool programIsSemanticallyValid = true;
        for (auto& stmt : parsed.program) {
            // Note: don't do a shortcut with && (e.g. valid && visit) since `&&` short circuits but visit has side
            // effects. thus, if the program is invalid visit will never run (since false && anything is false so the
            // right hand side isn't evaluated) which means that once one visit is invalid, all subsequent visits will
            // never happen (limiting the compiler to 1 error at a time)
            if (this->visit(stmt) == false) { programIsSemanticallyValid = false; }
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