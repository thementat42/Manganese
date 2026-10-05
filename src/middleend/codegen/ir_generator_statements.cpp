#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <core.hpp>
#include <frontend/ast.hpp>
#include <middleend/codegen/ir_generator.hpp>

namespace Manganese::codegen {

auto IRGenerator::visit(const ast::AggregateDeclarationStatement* statement) -> stmtvisit_t {
    if (!statement->genericTypes.empty()) { return; /*generate this on instantiation*/ }
    auto* llvmType = llvm::StructType::create(*context, statement->name);
    savedTypes[statement->semanticType]
        = llvmType;  // save immediately in case we have self-reference (pointer-to-self)

    std::vector<llvm::Type*> memberTypes;
    memberTypes.reserve(statement->fields.size());
    for (const auto& field : statement->fields) { memberTypes.push_back(visitTypeAsValue(field.type->semanticType)); }
    llvmType->setBody(memberTypes);
}

auto IRGenerator::visit(const ast::AliasStatement* /*unused*/) -> stmtvisit_t { /*type aliases are purely semantic*/ }

auto IRGenerator::visit(const ast::BreakStatement* /*unused*/) -> stmtvisit_t {
    builder->CreateBr(loopStack.top().exitBlock);
}

auto IRGenerator::visit(const ast::ContinueStatement* /*unused*/) -> stmtvisit_t {
    builder->CreateBr(loopStack.top().continueBlock);
}

auto IRGenerator::visit(const ast::EmptyStatement* /*unused*/) -> stmtvisit_t { /*doesn't need to do anything*/ }

auto IRGenerator::visit(const ast::EnumDeclarationStatement* /*unused*/) -> stmtvisit_t {
    /*
        don't need to do anything
        enum values are compile-time constants and emitted in scope resolution handling
    */
}

auto IRGenerator::visit(const ast::ExpressionStatement* statement) -> stmtvisit_t {
    DISCARD(visit(statement->expression));
}

auto IRGenerator::visit(const ast::ForLoopStatement* statement) -> stmtvisit_t {
    llvm::Function* function = builder->GetInsertBlock()->getParent();

    if (statement->initializationStep != nullptr) { visit(statement->initializationStep); }
    llvm::BasicBlock* conditionBlock = llvm::BasicBlock::Create(*context, "for_condition", function);
    llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(*context, "for_body", function);
    llvm::BasicBlock* postBlock = llvm::BasicBlock::Create(*context, "for_post", function);
    llvm::BasicBlock* exitBlock = llvm::BasicBlock::Create(*context, "for_exit", function);

    // jump to condition to check
    builder->CreateBr(conditionBlock);

    builder->SetInsertPoint(conditionBlock);
    if (statement->stopCondition != nullptr) {
        llvm::Value* condVal = visit(statement->stopCondition);
        builder->CreateCondBr(condVal, bodyBlock, exitBlock);
    } else {
        // Infinite loop if no condition is provided
        builder->CreateBr(bodyBlock);
    }

    // a for loop jumps to the post step on a continue
    loopStack.push({.continueBlock = postBlock, .exitBlock = exitBlock});

    // emit the body
    builder->SetInsertPoint(bodyBlock);
    visit(statement->body);
    if (builder->GetInsertBlock()->getTerminator() == nullptr) { builder->CreateBr(postBlock); }

    loopStack.pop();

    // run the post block then jump back to the condition
    builder->SetInsertPoint(postBlock);
    if (statement->postExpression != nullptr) { DISCARD(visit(statement->postExpression)); }
    builder->CreateBr(conditionBlock);
    builder->SetInsertPoint(exitBlock);
}

auto IRGenerator::visit(const ast::FunctionDeclarationStatement* statement) -> stmtvisit_t {
    if (!statement->genericTypes.empty()) { return; /*generate this on instantiation*/ }

    auto* functionType = llvm::cast<llvm::FunctionType>(visit(statement->semanticType));

    auto* llvmFunction
        = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, statement->mangledName, module.get());

    llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(*context, "entry", llvmFunction);
    builder->SetInsertPoint(entryBlock);

    llvm::Argument* currentLLVMArgument = llvmFunction->arg_begin();
    for (std::size_t i = 0; i < statement->parameters.size(); ++i, ++currentLLVMArgument) {
        const auto& param = statement->parameters[i];
        currentLLVMArgument->setName(param.name);

        llvm::AllocaInst* alloca = builder->CreateAlloca(currentLLVMArgument->getType(), nullptr, param.name + "_addr");
        builder->CreateStore(currentLLVMArgument, alloca);

        namedValues[param.name] = alloca;
    }

    visit(statement->body);
}

auto IRGenerator::visit(const ast::IfStatement* statement) -> stmtvisit_t {
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
        elifBlocks.emplace_back(elifConditionBlock, elifBodyBlock);
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

auto IRGenerator::visit(const ast::ImportStatement* /*unused*/) -> stmtvisit_t { /*doesn't mean anything in IR*/ }

auto IRGenerator::visit(const ast::ModuleDeclarationStatement* /*unused*/)
    -> stmtvisit_t { /*doesn't mean anything in IR*/ }

auto IRGenerator::visit(const ast::NamespaceStatement* statement) -> stmtvisit_t { visit(statement->block); }

auto IRGenerator::visit(const ast::NestedBlockStatement* statement) -> stmtvisit_t { visit(statement->block); }

auto IRGenerator::visit(const ast::ReturnStatement* statement) -> stmtvisit_t {
    if (statement->value == nullptr) {
        builder->CreateRetVoid();
        return;
    }
    llvm::Function* currentFunction = builder->GetInsertBlock()->getParent();
    llvm::Type* expectedReturnType = currentFunction->getReturnType();

    llvm::Value* returnValue = visit(statement->value);

    if (expectedReturnType->isStructTy() && returnValue->getType()->isPointerTy()) {
        returnValue = builder->CreateLoad(expectedReturnType, returnValue, "return");
    }

    builder->CreateRet(returnValue);
}

auto IRGenerator::visit(const ast::SwitchStatement* statement) -> stmtvisit_t {
    llvm::Function* function = builder->GetInsertBlock()->getParent();

    llvm::Value* condVal = visit(statement->target);

    // All cases eventually join to this block
    llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*context, "switch_end", function);

    // create the default block if there's a default body otherwise just the merge block
    llvm::BasicBlock* defaultBlock
        = statement->defaultBody.empty() ? mergeBlock : llvm::BasicBlock::Create(*context, "switch_default", function);

    // Pre-create basic blocks for each case
    struct CaseInfo {
        llvm::BasicBlock* block;
        const ast::CaseClause* astCase;
    };

    std::vector<CaseInfo> caseInfos;
    caseInfos.reserve(statement->cases.size());

    for (size_t i = 0; i < statement->cases.size(); ++i) {
        llvm::BasicBlock* caseBlk = llvm::BasicBlock::Create(*context, std::format("switch_case_{}", i), function);
        caseInfos.push_back({.block = caseBlk, .astCase = &statement->cases[i]});
    }

    // LLVM has a built-in switch which usually becomes a jump table
    llvm::SwitchInst* switchInstance
        = builder->CreateSwitch(condVal, defaultBlock, static_cast<unsigned>(statement->cases.size()));

    // Populate each case value and emit its body
    for (size_t i = 0; i < statement->cases.size(); ++i) {
        auto [caseBlock, astCase] = caseInfos[i];

        // A single case can often match multiple values (e.g., `case 1, 2, 3:`)
        for (const auto* valExpr : astCase->values) {
            auto* constVal = llvm::cast<llvm::ConstantInt>(visit(valExpr));
            switchInstance->addCase(constVal, caseBlock);
        }

        // Emit case body
        builder->SetInsertPoint(caseBlock);
        visit(astCase->body);

        // If the body doesn't end with a terminator (like a return or break), branch to merge
        if (builder->GetInsertBlock()->getTerminator() == nullptr) { builder->CreateBr(mergeBlock); }
    }

    // Emit default body if there is one
    if (!statement->defaultBody.empty()) {
        builder->SetInsertPoint(defaultBlock);
        visit(statement->defaultBody);
        if (builder->GetInsertBlock()->getTerminator() == nullptr) { builder->CreateBr(mergeBlock); }
    }

    builder->SetInsertPoint(mergeBlock);
}

auto IRGenerator::visit(const ast::VariableDeclarationStatement* statement) -> stmtvisit_t {
    llvm::Type* variableTypeLLVM = visitTypeAsValue(statement->semanticType);
    llvm::AllocaInst* alloca = builder->CreateAlloca(variableTypeLLVM, nullptr, statement->name);

    if (statement->value != nullptr && statement->value->kind != ast::ExpressionKind::UninitializedExpression) {
        llvm::Value* initializerValue = visit(statement->value);
        builder->CreateStore(initializerValue, alloca);
    }
    namedValues[std::string(statement->name)] = alloca;
}

auto IRGenerator::visit(const ast::WhileLoopStatement* statement) -> stmtvisit_t {
    llvm::Function* function = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* conditionBlock = llvm::BasicBlock::Create(*context, "while_loop_cond", function);
    llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(*context, "while_loop_body", function);
    llvm::BasicBlock* exitBlock = llvm::BasicBlock::Create(*context, "while_loop_exit", function);

    if (statement->isDoWhile) {
        // for a do/while loop, check the condition after the body
        builder->SetInsertPoint(conditionBlock);
        llvm::Value* condVal = visit(statement->condition);
        builder->CreateCondBr(condVal, bodyBlock, exitBlock);
    }

    if (statement->isDoWhile) {
        // do body first
        builder->CreateBr(bodyBlock);
        builder->SetInsertPoint(bodyBlock);
        visit(statement->body);
        loopStack.push({.continueBlock = conditionBlock, .exitBlock = exitBlock});

        // after we run the body, we check the condition
        builder->SetInsertPoint(conditionBlock);
        llvm::Value* condVal = visit(statement->condition);
        builder->CreateCondBr(condVal, bodyBlock, exitBlock);
        loopStack.pop();
    } else {
        // regular while loop, check the condition first
        builder->CreateBr(conditionBlock);

        // check condition
        builder->SetInsertPoint(conditionBlock);
        llvm::Value* condVal = visit(statement->condition);
        builder->CreateCondBr(condVal, bodyBlock, exitBlock);

        loopStack.push({.continueBlock = conditionBlock, .exitBlock = exitBlock});

        builder->SetInsertPoint(bodyBlock);
        visit(statement->body);
        if (builder->GetInsertBlock()->getTerminator() == nullptr) { builder->CreateBr(conditionBlock); }

        loopStack.pop();
    }
    builder->SetInsertPoint(exitBlock);
}

auto IRGenerator::visit(const ast::PoisonedStatement* /*unused*/) -> stmtvisit_t {
    ASSERT_UNREACHABLE("Poisoned statement was not flagged as semantically invalid during semantic analysis");
}

auto IRGenerator::visit(const ast::Block& block) -> stmtvisit_t {
    for (const ast::Statement* statement : block) { visit(statement); }
}

}  // namespace Manganese::codegen