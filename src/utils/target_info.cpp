#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

#include <io/logging.hpp>
#include <utils/target_info.hpp>


namespace Manganese {
TargetInfo TargetInfo::fromTriple(std::string_view tripleString) {
    llvm::Triple triple(tripleString);
    if (triple.isArch64Bit()) { return TargetInfo{.pointerSize = 8, .pointerAlignment = 8}; }
    if (triple.isArch32Bit()) { return TargetInfo{.pointerSize = 4, .pointerAlignment = 4}; }
    if (triple.isArch16Bit()) { return TargetInfo{.pointerSize = 2, .pointerAlignment = 2}; }

    logging::logCritical(0, 0, "Unsupported target architecture in triple: {}", tripleString);
    return TargetInfo{.pointerSize = 0, .pointerAlignment = 0};
}

TargetInfo TargetInfo::fromHostTriple() { return fromTriple(llvm::sys::getDefaultTargetTriple()); }

}  // namespace Manganese