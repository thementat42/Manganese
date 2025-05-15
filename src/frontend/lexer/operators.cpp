/**
 * @file operators.cpp
 * @brief This file contains the implementation of the operator functions for the Manganese compiler.
 */

#include "include/operators.h"

#include <optional>
#include <string>
#include <unordered_map>

#include "../../global_macros.h"

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

std::string operatorToString(OperatorType op) {
#if DEBUG
    switch (op) {
        case OperatorType::ADD:
            return "OperatorType::ADD";
        case OperatorType::SUB:
            return "OperatorType::SUB";
        case OperatorType::MUL:
            return "OperatorType::MUL";
        case OperatorType::DIV:
            return "OperatorType::DIV";
        case OperatorType::FLOOR_DIV:
            return "OperatorType::FLOOR_DIV";
        case OperatorType::MOD:
            return "OperatorType::MOD";
        case OperatorType::EXP:
            return "OperatorType::EXP";
        case OperatorType::INC:
            return "OperatorType::INC";
        case OperatorType::DEC:
            return "OperatorType::DEC";
        case OperatorType::ADD_ASSIGN:
            return "OperatorType::ADD_ASSIGN";
        case OperatorType::SUB_ASSIGN:
            return "OperatorType::SUB_ASSIGN";
        case OperatorType::MUL_ASSIGN:
            return "OperatorType::MUL_ASSIGN";
        case OperatorType::DIV_ASSIGN:
            return "OperatorType::DIV_ASSIGN";
        case OperatorType::FLOOR_DIV_ASSIGN:
            return "OperatorType::FLOOR_DIV_ASSIGN";
        case OperatorType::MOD_ASSIGN:
            return "OperatorType::MOD_ASSIGN";
        case OperatorType::EXP_ASSIGN:
            return "OperatorType::EXP_ASSIGN";
        case OperatorType::GT:
            return "OperatorType::GT";
        case OperatorType::GEQ:
            return "OperatorType::GEQ";
        case OperatorType::LT:
            return "OperatorType::LT";
        case OperatorType::LEQ:
            return "OperatorType::LEQ";
        case OperatorType::EQ:
            return "OperatorType::EQ";
        case OperatorType::NEQ:
            return "OperatorType::NEQ";
        case OperatorType::AND:
            return "OperatorType::AND";
        case OperatorType::OR:
            return "OperatorType::OR";
        case OperatorType::NOT:
            return "OperatorType::NOT";
        case OperatorType::BIT_AND:
            return "OperatorType::BIT_AND";
        case OperatorType::BIT_OR:
            return "OperatorType::BIT_OR";
        case OperatorType::BIT_NOT:
            return "OperatorType::BIT_NOT";
        case OperatorType::BIT_XOR:
            return "OperatorType::BIT_XOR";
        case OperatorType::BIT_LSHIFT:
            return "OperatorType::BIT_LSHIFT";
        case OperatorType::BIT_RSHIFT:
            return "OperatorType::BIT_RSHIFT";
        case OperatorType::BIT_AND_ASSIGN:
            return "OperatorType::BIT_AND_ASSIGN";
        case OperatorType::BIT_OR_ASSIGN:
            return "OperatorType::BIT_OR_ASSIGN";
        case OperatorType::BIT_NOT_ASSIGN:
            return "OperatorType::BIT_NOT_ASSIGN";
        case OperatorType::BIT_XOR_ASSIGN:
            return "OperatorType::BIT_XOR_ASSIGN";
        case OperatorType::BIT_LSHIFT_ASSIGN:
            return "OperatorType::BIT_LSHIFT_ASSIGN";
        case OperatorType::BIT_RSHIFT_ASSIGN:
            return "OperatorType::BIT_RSHIFT_ASSIGN";
        case OperatorType::ADDRESS:
            return "OperatorType::ADDRESS";
        case OperatorType::DEREF:
            return "OperatorType::DEREF";
        case OperatorType::MEMBER:
            return "OperatorType::MEMBER";
        case OperatorType::ELLIPSIS:
            return "OperatorType::ELLIPSIS";
        case OperatorType::SCOPE_RESOLUTION:
            return "OperatorType::SCOPE_RESOLUTION";
        case OperatorType::ASSIGNMENT:
            return "OperatorType::ASSIGNMENT";
        case OperatorType::ARROW:
            return "OperatorType::ARROW";
        default:
            return "Unknown Operator";
    }
#else  // ^ DEBUG ^ | v !DEBUG v
    return "";
#endif // DEBUG
}

std::unordered_map<std::string, const OperatorType> operator_map = {
    // Arithmetic Operators
    {"+", OperatorType::ADD},
    {"-", OperatorType::SUB},
    {"*", OperatorType::MUL},
    {"/", OperatorType::DIV},
    {"//", OperatorType::FLOOR_DIV},
    {"%", OperatorType::MOD},
    {"**", OperatorType::EXP},
    {"++", OperatorType::INC},
    {"--", OperatorType::DEC},

    // Arithmetic Assignment Operators
    {"+=", OperatorType::ADD_ASSIGN},
    {"-=", OperatorType::SUB_ASSIGN},
    {"*=", OperatorType::MUL_ASSIGN},
    {"/=", OperatorType::DIV_ASSIGN},
    {"//=", OperatorType::FLOOR_DIV_ASSIGN},
    {"%=", OperatorType::MOD_ASSIGN},
    {"**=", OperatorType::EXP_ASSIGN},

    // Comparison Operators
    {">", OperatorType::GT},
    {">=", OperatorType::GEQ},
    {"<", OperatorType::LT},
    {"<=", OperatorType::LEQ},
    {"==", OperatorType::EQ},
    {"!=", OperatorType::NEQ},

    // Boolean Operators
    {"&&", OperatorType::AND},
    {"||", OperatorType::OR},
    {"!", OperatorType::NOT},

    // Bitwise Operators
    {"&", OperatorType::BIT_AND},
    {"|", OperatorType::BIT_OR},
    {"~", OperatorType::BIT_NOT},
    {"^", OperatorType::BIT_XOR},
    {"<<", OperatorType::BIT_LSHIFT},
    {">>", OperatorType::BIT_RSHIFT},

    // Bitwise Assignment Operators
    {"&=", OperatorType::BIT_AND_ASSIGN},
    {"|=", OperatorType::BIT_OR_ASSIGN},
    {"~=", OperatorType::BIT_NOT_ASSIGN},
    {"^=", OperatorType::BIT_XOR_ASSIGN},
    {"<<=", OperatorType::BIT_LSHIFT_ASSIGN},
    {">>=", OperatorType::BIT_RSHIFT_ASSIGN},

    // Pointer Operators
    {"?", OperatorType::ADDRESS},
    {"@", OperatorType::DEREF},

    // Access Operators
    {".", OperatorType::MEMBER},
    {"...", OperatorType::ELLIPSIS},
    {"::", OperatorType::SCOPE_RESOLUTION},

    // Misc
    {"=", OperatorType::ASSIGNMENT},
    {"->", OperatorType::ARROW}};

}  // namespace lexer
MANG_END
