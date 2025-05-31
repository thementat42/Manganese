/**
 * @file operators.cpp
 * @brief This file contains the implementation of the operator functions for the Manganese compiler.
 */

#include "include/operators.h"

#include <optional>
#include <string>
#include <unordered_map>

#include "../global_macros.h"

namespace manganese {
namespace core {

std::optional<OperatorType> operatorFromString(const std::string& op) {
    std::string op_str(op);
    auto it = operatorMap.find(op_str);
    if (it != operatorMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

DEBUG_FUNC std::optional<std::string> operatorToString(std::optional<OperatorType> op) {
    if (!op.has_value()) {
        return std::nullopt;
    }
#if DEBUG
    switch (*op) {
        case OperatorType::Plus:
            return "+";
        case OperatorType::Minus:
            return "-";
        case OperatorType::Mul:
            return "*";
        case OperatorType::Div:
            return "/";
        case OperatorType::FloorDiv:
            return "//";
        case OperatorType::Mod:
            return "%";
        case OperatorType::Exp:
            return "**";
        case OperatorType::Inc:
            return "++";
        case OperatorType::Dec:
            return "--";
        case OperatorType::PlusAssign:
            return "+=";
        case OperatorType::MinusAssign:
            return "-=";
        case OperatorType::MulAssign:
            return "*=";
        case OperatorType::DivAssign:
            return "/=";
        case OperatorType::FloorDivAssign:
            return "//=";
        case OperatorType::ModAssign:
            return "%=";
        case OperatorType::ExpAssign:
            return "**=";
        case OperatorType::GreaterThan:
            return ">";
        case OperatorType::GreaterThanOrEqual:
            return ">=";
        case OperatorType::LessThan:
            return "<";
        case OperatorType::LessThanOrEqual:
            return "<=";
        case OperatorType::Equal:
            return "==";
        case OperatorType::NotEqual:
            return "!=";
        case OperatorType::And:
            return "&&";
        case OperatorType::Or:
            return "||";
        case OperatorType::Not:
            return "!";
        case OperatorType::BitAnd:
            return "&";
        case OperatorType::BitOr:
            return "|";
        case OperatorType::BitNot:
            return "~";
        case OperatorType::BitXor:
            return "^";
        case OperatorType::BitLShift:
            return "<<";
        case OperatorType::BitRShift:
            return ">>";
        case OperatorType::BitAndAssign:
            return "&=";
        case OperatorType::BitOrAssign:
            return "|=";
        case OperatorType::BitNotAssign:
            return "~=";
        case OperatorType::BitXorAssign:
            return "^=";
        case OperatorType::BitLShiftAssign:
            return "<<=";
        case OperatorType::BitRShiftAssign:
            return ">>=";
        case OperatorType::AddressOf:
            return "?";
        case OperatorType::Dereference:
            return "@";
        case OperatorType::MemberAccess:
            return ".";
        case OperatorType::Ellipsis:
            return "...";
        case OperatorType::ScopeResolution:
            return "::";
        case OperatorType::Assignment:
            return "=";
        case OperatorType::Arrow:
            return "->";
        default:
            return "Unknown Operator";
    }
#else   // ^ DEBUG ^ | v !DEBUG v
    return "";
#endif  // DEBUG
}

std::unordered_map<std::string, const OperatorType> operatorMap = {
    // Arithmetic Operators
    {"+", OperatorType::Plus},
    {"-", OperatorType::Minus},
    {"*", OperatorType::Mul},
    {"/", OperatorType::Div},
    {"//", OperatorType::FloorDiv},
    {"%", OperatorType::Mod},
    {"**", OperatorType::Exp},
    {"++", OperatorType::Inc},
    {"--", OperatorType::Dec},

    // Arithmetic Assignment Operators
    {"+=", OperatorType::PlusAssign},
    {"-=", OperatorType::MinusAssign},
    {"*=", OperatorType::MulAssign},
    {"/=", OperatorType::DivAssign},
    {"//=", OperatorType::FloorDivAssign},
    {"%=", OperatorType::ModAssign},
    {"**=", OperatorType::ExpAssign},

    // Comparison Operators
    {">", OperatorType::GreaterThan},
    {">=", OperatorType::GreaterThanOrEqual},
    {"<", OperatorType::LessThan},
    {"<=", OperatorType::LessThanOrEqual},
    {"==", OperatorType::Equal},
    {"!=", OperatorType::NotEqual},

    // Boolean Operators
    {"&&", OperatorType::And},
    {"||", OperatorType::Or},
    {"!", OperatorType::Not},

    // Bitwise Operators
    {"&", OperatorType::BitAnd},
    {"|", OperatorType::BitOr},
    {"~", OperatorType::BitNot},
    {"^", OperatorType::BitXor},
    {"<<", OperatorType::BitLShift},
    {">>", OperatorType::BitRShift},

    // Bitwise Assignment Operators
    {"&=", OperatorType::BitAndAssign},
    {"|=", OperatorType::BitOrAssign},
    {"~=", OperatorType::BitNotAssign},
    {"^=", OperatorType::BitXorAssign},
    {"<<=", OperatorType::BitLShiftAssign},
    {">>=", OperatorType::BitRShiftAssign},

    // Pointer Operators
    {"?", OperatorType::AddressOf},
    {"@", OperatorType::Dereference},

    // Access Operators
    {".", OperatorType::MemberAccess},
    {"...", OperatorType::Ellipsis},
    {"::", OperatorType::ScopeResolution},

    // Misc
    {"=", OperatorType::Assignment},
    {"->", OperatorType::Arrow}};

}  // namespace core
}
