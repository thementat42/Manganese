#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <backend/object_code_emitter.hpp>
#include <core.hpp>
#include <utils/result.hpp>
#include <utils/target_info.hpp>

namespace Manganese::backend {

Result emitObjectCode(llvm::Module* module, utils::TargetInfo& targetInfo, std::string_view filename) {
    initializeLLVM();

    if (targetInfo.targetMachine == nullptr) {
        ASSERT_UNREACHABLE("TargetMachine not set before emitting object code");
    }
    auto targetTriple = targetInfo.targetMachine->getTargetTriple();
    module->setTargetTriple(targetTriple);

    std::string error;

    const auto* target = llvm::TargetRegistry::lookupTarget(module->getTargetTriple(), error);

    if (target == nullptr) {
        llvm::errs() << error;
        return Result::Failure;
    }

    llvm::TargetOptions opt;

    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);

    if (ec) {
        llvm::errs() << "Could not open file: " << ec.message();
        return Result::Failure;
    }

    llvm::legacy::PassManager passManager;
    auto fileType = llvm::CodeGenFileType::ObjectFile;

    if (targetInfo.targetMachine->addPassesToEmitFile(passManager, dest, nullptr, fileType)) {
        llvm::errs() << "The target machine can't emit a file of this type";
        return Result::Failure;
    }

    passManager.run(*module);
    dest.flush();

    return Result::Success;
}

}  // namespace Manganese::backend