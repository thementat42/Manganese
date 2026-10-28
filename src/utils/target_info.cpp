#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

#include <io/logging.hpp>
#include <string_view>
#include <utils/target_info.hpp>

namespace Manganese::utils {
TargetInfo TargetInfo::fromTriple(std::string_view tripleString) {
    const llvm::Triple triple{llvm::StringRef(tripleString.data(), tripleString.size())};

    std::size_t pointerSize = 0;
    std::size_t pointerAlignment = 0;

    if (triple.isArch64Bit()) {
        pointerSize = 8;
        pointerAlignment = 8;
    } else if (triple.isArch32Bit()) {
        pointerSize = 4;
        pointerAlignment = 4;
    } else if (triple.isArch16Bit()) {
        pointerSize = 2;
        pointerAlignment = 2;
    } else {
        logging::logCritical(0, 0, "Unsupported target architecture in triple: {}", tripleString);
        return TargetInfo{.pointerSize = 0, .pointerAlignment = 0, .targetMachine = nullptr};
    }

    std::string error;
    // Pass the Triple reference directly to lookupTarget
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple, error);

    std::unique_ptr<llvm::TargetMachine> tm = nullptr;
    if (target != nullptr) {
        llvm::TargetOptions opt;
        tm = std::unique_ptr<llvm::TargetMachine>(target->createTargetMachine(
            triple, "", "", opt, std::nullopt, std::nullopt, llvm::CodeGenOptLevel::Default, false));
    } else {
        logging::logCritical(0, 0, "Failed to lookup LLVM target for triple '{}': {}", tripleString, error);
    }

    return TargetInfo{.pointerSize = pointerSize, .pointerAlignment = pointerAlignment, .targetMachine = std::move(tm)};
}

TargetInfo TargetInfo::fromHostTriple() { return fromTriple(llvm::sys::getDefaultTargetTriple()); }

}  // namespace Manganese::utils