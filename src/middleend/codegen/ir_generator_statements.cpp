#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <core.hpp>
#include <frontend/ast.hpp>
#include <middleend/codegen/ir_generator.hpp>

namespace Manganese::codegen {

auto IRGenerator::visit([[maybe_unused]] const ast::AggregateDeclarationStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::AliasStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::BreakStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::ContinueStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit(const ast::EmptyStatement* /*unused*/) -> stmtvisit_t { /*doesn't need to do anything*/ }

auto IRGenerator::visit([[maybe_unused]] const ast::EnumDeclarationStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::ExpressionStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::ForLoopStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::FunctionDeclarationStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::IfStatement* statement) -> stmtvisit_t {
    llvm::Function* function = builder->GetInsertBlock()->getParent();

    // set up blocks for each place to jump to
    // merge block is where to jump after the if statement
    llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*context, "if_end", function);
    // block for the main if statement
    llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(*context, "if_true", function);

    std::vector<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> elifBlocks;
    elifBlocks.reserve(statement->elifs.size());

    for (std::size_t i = 0; i < statement->elifs.size(); ++i) {
        llvm::BasicBlock* elifConditionBlock
            = llvm::BasicBlock::Create(*context, std::format("elif_condition_{}", i), function);
        llvm::BasicBlock* elifBodyBlock
            = llvm::BasicBlock::Create(*context, std::format("elif_condition_{}_true", i), function);
        elifBlocks.push_back({elifConditionBlock, elifBodyBlock});
    }

    llvm::BasicBlock* elseBlock
        = statement->elseBody.empty() ? nullptr : llvm::BasicBlock::Create(*context, "else_body", function);

    // Determine where the primary 'if' false branch should jump
    // If there are elifs, jump to the first one, otherwise we can just go straight to else
    llvm::BasicBlock* mainFalseDest
        = !elifBlocks.empty() ? elifBlocks[0].first : ((elseBlock != nullptr) ? elseBlock : mergeBlock);

    // Run codegen for the main if statement
    llvm::Value* condVal = visit(statement->condition);
    builder->CreateCondBr(condVal, thenBlock, mainFalseDest);

    builder->SetInsertPoint(thenBlock);
    visit(statement->body);
    if (builder->GetInsertBlock()->getTerminator() == nullptr) { builder->CreateBr(mergeBlock); }

    // emit each elif branch

    for (size_t i = 0; i < statement->elifs.size(); ++i) {
        auto [condBlk, bodyBlk] = elifBlocks[i];

        // If this elif fails, jump to the next elif, the else block, or the merge block
        llvm::BasicBlock* nextFalseDest
            = (i + 1 < elifBlocks.size()) ? elifBlocks[i + 1].first : ((elseBlock != nullptr) ? elseBlock : mergeBlock);

        // Emit the elif condition block
        builder->SetInsertPoint(condBlk);
        llvm::Value* elifCondVal = visit(statement->elifs[i].condition);
        builder->CreateCondBr(elifCondVal, bodyBlk, nextFalseDest);

        // Emit the elif body block
        builder->SetInsertPoint(bodyBlk);
        visit(statement->elifs[i].body);
        if (builder->GetInsertBlock()->getTerminator() == nullptr) { builder->CreateBr(mergeBlock); }
    }

    if (elseBlock != nullptr) {
        builder->SetInsertPoint(elseBlock);
        visit(statement->elseBody);
        if (builder->GetInsertBlock()->getTerminator() == nullptr) { builder->CreateBr(mergeBlock); }
    }

    // done with the if statement, we want to go back to emitting code outside it
    builder->SetInsertPoint(mergeBlock);
}

auto IRGenerator::visit([[maybe_unused]] const ast::ImportStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::ModuleDeclarationStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit(const ast::NamespaceStatement* statement) -> stmtvisit_t { visit(statement->block); }

auto IRGenerator::visit(const ast::NestedBlockStatement* statement) -> stmtvisit_t { visit(statement->block); }

auto IRGenerator::visit([[maybe_unused]] const ast::ReturnStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::SwitchStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::VariableDeclarationStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit([[maybe_unused]] const ast::WhileLoopStatement* statement) -> stmtvisit_t {}

auto IRGenerator::visit(const ast::PoisonedStatement* /*unused*/) -> stmtvisit_t {
    ASSERT_UNREACHABLE("Poisoned statement was not flagged as semantically invalid during semantic analysis");
}

auto IRGenerator::visit(const ast::Block& block) -> stmtvisit_t {
    for (const ast::Statement* statement : block) { visit(statement); }
}

}  // namespace Manganese::codegen