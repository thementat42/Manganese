#ifndef MANGANESE_INCLUDE_UTILS_TARGET_INFO_HPP
#define MANGANESE_INCLUDE_UTILS_TARGET_INFO_HPP 1

#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace Manganese::utils {
struct TargetInfo {
    std::size_t pointerSize;
    std::size_t pointerAlignment;
    std::unique_ptr<llvm::TargetMachine> targetMachine;

    static TargetInfo fromTriple(std::string_view triple);
    static TargetInfo fromHostTriple();
};

inline TargetInfo& getTargetInfo() {
    static utils::TargetInfo targetInfo = utils::TargetInfo::fromHostTriple();
    return targetInfo;
}
}  // namespace Manganese::utils

#endif  // MANGANESE_INCLUDE_UTILS_TARGET_INFO_HPP