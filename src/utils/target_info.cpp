#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

#include <io/logging.hpp>
#include <string_view>
#include <utils/target_info.hpp>

namespace Manganese {
utils::TargetInfo utils::TargetInfo::fromTriple(std::string_view tripleString) {
    const llvm::Triple triple{tripleString};
    if (triple.isArch64Bit()) { return utils::TargetInfo{.pointerSize = 8, .pointerAlignment = 8}; }
    if (triple.isArch32Bit()) { return utils::TargetInfo{.pointerSize = 4, .pointerAlignment = 4}; }
    if (triple.isArch16Bit()) { return utils::TargetInfo{.pointerSize = 2, .pointerAlignment = 2}; }

    logging::logCritical(0, 0, "Unsupported target architecture in triple: {}", tripleString);
    return utils::TargetInfo{.pointerSize = 0, .pointerAlignment = 0};
}

utils::TargetInfo utils::TargetInfo::fromHostTriple() { return fromTriple(llvm::sys::getDefaultTargetTriple()); }

}  // namespace Manganese