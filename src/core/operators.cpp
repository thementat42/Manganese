/**
 * @file operators.cpp
 * @brief This file contains the implementation of the operator functions for the Manganese compiler.
 */

#include "include/operators.h"

#include <optional>
#include <string>
#include <unordered_map>

#include "../global_macros.h"

MANG_BEGIN
namespace lexer {

std::optional<OperatorType> operatorFromString(const std::string& op) {
    std::string op_str(op);
    auto it = operator_map.find(op_str);
    if (it != operator_map.end()) {
        return it->second;
    }
    return std::nullopt;
}

DEBUG_FUNC std::string operatorToString(std::optional<OperatorType> op) {
    if (!op.has_value()) {
        return "Invalid Operator";
    }
#if DEBUG
    switch (*op) {
        case OperatorType::Plus:
            return "OperatorType::Plus";
        case OperatorType::Minus:
            return "OperatorType::Minus";
        case OperatorType::Mul:
            return "OperatorType::Mul";
        case OperatorType::Div:
            return "OperatorType::Div";
        case OperatorType::FloorDiv:
            return "OperatorType::FloorDiv";
        case OperatorType::Mod:
            return "OperatorType::Mod";
        case OperatorType::Exp:
            return "OperatorType::Exp";
        case OperatorType::Inc:
            return "OperatorType::Inc";
        case OperatorType::Dec:
            return "OperatorType::Dec";
        case OperatorType::PlusAssign:
            return "OperatorType::PlusAssign";
        case OperatorType::MinusAssign:
            return "OperatorType::MinusAssign";
        case OperatorType::MulAssign:
            return "OperatorType::MulAssign";
        case OperatorType::DivAssign:
            return "OperatorType::DivAssign";
        case OperatorType::FloorDivAssign:
            return "OperatorType::FloorDivAssign";
        case OperatorType::ModAssign:
            return "OperatorType::ModAssign";
        case OperatorType::ExpAssign:
            return "OperatorType::ExpAssign";
        case OperatorType::GreaterThan:
            return "OperatorType::GreaterThan";
        case OperatorType::GreaterThanOrEqual:
            return "OperatorType::GreaterThanOrEqual";
        case OperatorType::LessThan:
            return "OperatorType::LessThan";
        case OperatorType::LessThanOrEqual:
            return "OperatorType::LessThanOrEqual";
        case OperatorType::Equal:
            return "OperatorType::Equal";
        case OperatorType::NotEqual:
            return "OperatorType::NotEqual";
        case OperatorType::And:
            return "OperatorType::And";
        case OperatorType::Or:
            return "OperatorType::Or";
        case OperatorType::Not:
            return "OperatorType::Not";
        case OperatorType::BitAnd:
            return "OperatorType::BitAnd";
        case OperatorType::BitOr:
            return "OperatorType::BitOr";
        case OperatorType::BitNot:
            return "OperatorType::BitNot";
        case OperatorType::BitXor:
            return "OperatorType::BitXor";
        case OperatorType::BitLShift:
            return "OperatorType::BitLShift";
        case OperatorType::BitRShift:
            return "OperatorType::BitRShift";
        case OperatorType::BitAndAssign:
            return "OperatorType::BitAndAssign";
        case OperatorType::BitOrAssign:
            return "OperatorType::BitOrAssign";
        case OperatorType::BitNotAssign:
            return "OperatorType::BitNotAssign";
        case OperatorType::BitXorAssign:
            return "OperatorType::BitXorAssign";
        case OperatorType::BitLShiftAssign:
            return "OperatorType::BitLShiftAssign";
        case OperatorType::BitRShiftAssign:
            return "OperatorType::BitRShiftAssign";
        case OperatorType::AddressOf:
            return "OperatorType::AddressOf";
        case OperatorType::Dereference:
            return "OperatorType::Dereference";
        case OperatorType::MemberAccess:
            return "OperatorType::MemberAccess";
        case OperatorType::Ellipsis:
            return "OperatorType::Ellipsis";
        case OperatorType::ScopeResolution:
            return "OperatorType::ScopeResolution";
        case OperatorType::Assignment:
            return "OperatorType::Assignment";
        case OperatorType::Arrow:
            return "OperatorType::Arrow";
        default:
            return "Unknown Operator";
    }
#else   // ^ DEBUG ^ | v !DEBUG v
    return "";
#endif  // DEBUG
}

std::unordered_map<std::string, const OperatorType> operator_map = {
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

}  // namespace lexer
MANG_END
