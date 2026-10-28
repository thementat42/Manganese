#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/CodeGen/MachineFunction.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Target/TargetMachine.h>

#include <middleend/optimizations.hpp>


// https://llvm.org/docs/NewPassManager.html

namespace Manganese::codegen {
void optimizeModule(llvm::Module* module, llvm::OptimizationLevel level, llvm::TargetMachine* tm) {
    if (level == llvm::OptimizationLevel::O0) { return; }
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

    llvm::PassBuilder passBuilder{tm};

    passBuilder.registerModuleAnalyses(MAM);
    passBuilder.registerCGSCCAnalyses(CGAM);
    passBuilder.registerFunctionAnalyses(FAM);
    passBuilder.registerLoopAnalyses(LAM);
    passBuilder.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    if (tm != nullptr) { tm->registerPassBuilderCallbacks(passBuilder); }

    llvm::ModulePassManager MPM = passBuilder.buildPerModuleDefaultPipeline(level);

    MPM.run(*module, MAM);
}
}  // namespace Manganese::codegen