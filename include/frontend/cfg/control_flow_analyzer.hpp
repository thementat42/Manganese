
#ifndef MANGANESE_INCLUDE_FRONTEND_CFG_CONTROL_FLOW_ANALYZER_HPP
#define MANGANESE_INCLUDE_FRONTEND_CFG_CONTROL_FLOW_ANALYZER_HPP 1

#include <cstdint>
#include <frontend/ast.hpp>
#include <frontend/ast/visitor_base.hpp>
#include <frontend/parser.hpp>
#include <frontend/semantic.hpp>
#include <io/logging.hpp>
#include <mnstl/chunk_allocator.hxx>
#include <utils/result.hpp>
#include <utils/target_info.hpp>
#include <vector>

namespace Manganese::cfg {

enum class FlowStatus : std::int8_t {
    FallsThrough = -1,
    Returns = 1,
    Breaks = 2,
    ContinuesLoop = 3,
    InfiniteLoop = 4
};

enum class AssignmentState : std::int8_t {
    Uninitialized = -1,
    MaybeInitialized = 0,
    Initialized = 1
};

using _flow_base_t = ast::Visitor<void, FlowStatus, void, false>;

class ControlFlowAnalyzer final : public _flow_base_t {
   private:
    const std::vector<parser::ParsedFile>& files;
    const utils::TargetInfo& targetInfo;
    const semantic::SymbolTable& symbolTable;  // need to look up symbols for definite assignment
    std::vector<AssignmentState> symbolAssignmentStates;  // indexed into by Symbol::ID

    struct {
        bool hasError : 1 = false;
        bool hasWarning : 1 = false;
    } flags;

   public:
    ControlFlowAnalyzer(const std::vector<parser::ParsedFile>& _files, utils::TargetInfo& _targetInfo,
                        const semantic::SymbolTable& _symbolTable) :
        files(_files), targetInfo(_targetInfo), symbolTable(_symbolTable) {
        symbolAssignmentStates.resize(symbolTable.getSize(), AssignmentState::Uninitialized);
    }
    Result analyze() {
        for (const auto& file : files) {
            for (const ast::Statement* statement : file.program) { visit(statement); }
        }
        return flags.hasError ? Result::Failure : Result::Success;
    }

   private:
    template <class... Args>
    void logError(const ast::ASTNode* node, std::format_string<Args...> message, Args&&... args) noexcept {
        logging::logError(node->line, node->column, message, std::forward<Args>(args)...);
        flags.hasError = true;
    }

    template <class... Args>
    void logWarning(const ast::ASTNode* node, std::format_string<Args...> message, Args&&... args) noexcept {
        logging::logWarning(node->line, node->column, message, std::forward<Args>(args)...);
        flags.hasWarning = true;
    }

   protected:
    // overrides for visitor functions

    using _flow_base_t::visit;

#define STMT(name) stmtvisit_t visit(const ast::name*) noexcept override;

#define EXPR(name) exprvisit_t visit(const ast::name*) noexcept override;
    // Expressions and types don't contribute to control flow or assignment so they can just be no-ops
#define TYPE(name) \
    constexpr typevisit_t visit(const ast::name*) noexcept override {}

#include <frontend/ast/ast.def>

#undef STMT
#undef EXPR
#undef TYPE

    FlowStatus visit(const ast::Block&);
};

}  // namespace Manganese::cfg

#endif  // MANGANESE_INCLUDE_FRONTEND_CFG_CONTROL_FLOW_ANALYZER_HPP