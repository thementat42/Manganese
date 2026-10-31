#ifndef MANGANESE_INCLUDE_BACKEND_OBJECT_CODE_EMITTER_HPP
#define MANGANESE_INCLUDE_BACKEND_OBJECT_CODE_EMITTER_HPP 1

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <core.hpp>
#include <utils/result.hpp>
#include <utils/target_info.hpp>

namespace Manganese::backend {

inline void initializeLLVM() {
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();
}

// https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl08.html

Result emitObjectCode(llvm::Module* module, utils::TargetInfo& targetInfo, std::string_view filename);

}  // namespace Manganese::backend

#endif  // MANGANESE_INCLUDE_BACKEND_OBJECT_CODE_EMITTER_HPP