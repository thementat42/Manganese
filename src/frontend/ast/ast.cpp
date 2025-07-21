/**
 * @file ast.cpp
 * @brief Implementation of some AST node methods.
 *
 * @note This file is implementations of any AST node methods which are get too complex to be inline
 */
#include <frontend/ast.h>
#include <global_macros.h>

#include <memory>
#include <string>
#include <utility>

namespace Manganese {
namespace ast {

std::string visibilityToString(const Visibility visibility) noexcept_if_release {
    switch (visibility) {
        case Visibility::Public:
            return "public ";
            break;
        case Visibility::ReadOnly:
            return "readonly ";
            break;
        case Visibility::Private:
            return "private ";
            break;
        default:
            ASSERT_UNREACHABLE("Invalid visibility");
    }
}
}  // namespace ast
}  // namespace Manganese
