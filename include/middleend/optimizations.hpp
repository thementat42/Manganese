#ifndef MANGANESE_INCLUDE_MIDDLEEND_OPTIMIZATIONS_OPTIMIZATIONS_HPP
#define MANGANESE_INCLUDE_MIDDLEEND_OPTIMIZATIONS_OPTIMIZATIONS_HPP 1

#include <llvm/IR/Module.h>
#include <llvm/Passes/OptimizationLevel.h>

namespace Manganese::codegen {
void optimizeModule(llvm::Module* module, llvm::OptimizationLevel level);
}

#endif  // MANGANESE_INCLUDE_MIDDLEEND_OPTIMIZATIONS_OPTIMIZATIONS_HPP