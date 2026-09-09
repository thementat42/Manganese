
#ifndef MANGANESE_INCLUDE_FRONTEND_CFG_CONTROL_FLOW_ANALYZER_HPP
#define MANGANESE_INCLUDE_FRONTEND_CFG_CONTROL_FLOW_ANALYZER_HPP 1

#include <cstdint>
#include <frontend/ast.hpp>
#include <frontend/ast/visitor_base.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic.hpp>
#include <mnstl/chunk_allocator.hxx>
#include <utils/result.hpp>
#include <utils/target_info.hpp>

namespace Manganese::cfg {

enum class FlowStatus : std::int8_t {
    FallsThrough = -1,
    Returns = 1,
    Breaks = 2,
    ContinuesLoop = 3
};

using _flow_base_t = ast::Visitor<void, FlowStatus, void, false>;

class ControlFlowAnalyzer final : public _flow_base_t {
   private:
    semantic::SemanticAnalyzer semanticAnalyzer;

   public:
    ControlFlowAnalyzer(std::vector<parser::ParsedFile>& parsedFiles, const utils::TargetInfo& targetInfo,
                        mnstl::chunk_allocator& allocator) :
        semanticAnalyzer(parsedFiles, targetInfo, allocator) {}
    Result analyze();

   protected:
    // overrides for visitor functions

#define STMT(name) stmtvisit_t visit(const ast::name*) override;

// Expressions and types don't contribute to control flow so they can just immediately return
#define EXPR(name) \
    exprvisit_t visit(const ast::name*) override {}
#define TYPE(name) \
    typevisit_t visit(const ast::name*) override {}

#include <frontend/ast/ast.def>

#undef STMT
#undef EXPR
#undef TYPE
};

}  // namespace Manganese::cfg

#endif  // MANGANESE_INCLUDE_FRONTEND_CFG_CONTROL_FLOW_ANALYZER_HPP