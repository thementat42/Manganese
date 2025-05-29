/**
 * @file keywords.cpp
 * @brief This file contains the implementation of the keyword functions for the Manganese compiler.
 */

#include "include/keywords.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "../global_macros.h"

MANG_BEGIN
namespace core {

std::optional<KeywordType> keywordFromString(const std::string& keyword) {
    auto it = keywordMap.find(keyword);
    if (it != keywordMap.end()) {
        return it->second;
    }

    return std::nullopt;
}

DEBUG_FUNC std::string keywordToString(KeywordType keyword) {
#if DEBUG
    switch (keyword) {
        case KeywordType::Alias:
            return "alias";
        case KeywordType::As:
            return "as";
        case KeywordType::Blueprint:
            return "blueprint";
        case KeywordType::Bool:
            return "bool";
        case KeywordType::Break:
            return "break";
        case KeywordType::Bundle:
            return "bundle";
        case KeywordType::Case:
            return "case";
        case KeywordType::Cast:
            return "cast";
        case KeywordType::Char:
            return "char";
        case KeywordType::Const:
            return "const";
        case KeywordType::Continue:
            return "continue";
        case KeywordType::Default:
            return "Default";
        case KeywordType::Do:
            return "do";
        case KeywordType::Elif:
            return "elif";
        case KeywordType::Else:
            return "else";
        case KeywordType::Enum:
            return "enum";
        case KeywordType::False:
            return "false";
        case KeywordType::Float:
            return "float";
        case KeywordType::Float32:
            return "float32";
        case KeywordType::Float64:
            return "float64";
        case KeywordType::For:
            return "for";
        case KeywordType::Func:
            return "func";
        case KeywordType::If:
            return "if";
        case KeywordType::Import:
            return "import";
        case KeywordType::Int8:
            return "int8";
        case KeywordType::Int16:
            return "int16";
        case KeywordType::Int32:
            return "int32";
        case KeywordType::Int64:
            return "int64";
        case KeywordType::Lambda:
            return "lambda";
        case KeywordType::Module:
            return "module";
        case KeywordType::Ptr:
            return "ptr";
        case KeywordType::Public:
            return "public";
        case KeywordType::ReadOnly:
            return "readonly";
        case KeywordType::Repeat:
            return "repeat";
        case KeywordType::Return:
            return "return";
        case KeywordType::Switch:
            return "switch";
        case KeywordType::True:
            return "true";
        case KeywordType::TypeOf:
            return "typeof";
        case KeywordType::UInt8:
            return "uint8";
        case KeywordType::UInt16:
            return "uint16";
        case KeywordType::UInt32:
            return "uint32";
        case KeywordType::UInt64:
            return "uint64";
        case KeywordType::While:
            return "while";
        default:
            return "Unknown Keyword";
    }
#else  // ^ DEBUG ^ | v !DEBUG v
    return "";
#endif // DEBUG
}

std::unordered_map<std::string, const KeywordType> keywordMap = {
    {"alias", KeywordType::Alias},
    {"as", KeywordType::As},
    {"blueprint", KeywordType::Blueprint},
    {"bool", KeywordType::Bool},
    {"break", KeywordType::Break},
    {"bundle", KeywordType::Bundle},
    {"case", KeywordType::Case},
    {"cast", KeywordType::Cast},
    {"char", KeywordType::Char},
    {"const", KeywordType::Const},
    {"continue", KeywordType::Continue},
    {"default", KeywordType::Default},
    {"do", KeywordType::Do},
    {"elif", KeywordType::Elif},
    {"else", KeywordType::Else},
    {"enum", KeywordType::Enum},
    {"false", KeywordType::False},
    {"float", KeywordType::Float},  // default to float32 when floating point width isn't specified
    {"float32", KeywordType::Float32},
    {"float64", KeywordType::Float64},
    {"for", KeywordType::For},
    {"func", KeywordType::Func},
    {"if", KeywordType::If},
    {"import", KeywordType::Import},
    {"int", KeywordType::Int32},  // default to int32 when integer width isn't specified
    {"int16", KeywordType::Int16},
    {"int32", KeywordType::Int32},
    {"int64", KeywordType::Int64},
    {"int8", KeywordType::Int8},
    {"lambda", KeywordType::Lambda},
    {"module", KeywordType::Module},
    {"ptr", KeywordType::Ptr},
    {"public", KeywordType::Public},
    {"readonly", KeywordType::ReadOnly},
    {"repeat", KeywordType::Repeat},
    {"return", KeywordType::Return},
    {"switch", KeywordType::Switch},
    {"true", KeywordType::True},
    {"typeof", KeywordType::TypeOf},
    {"uint", KeywordType::UInt32},  // default to uint32 when integer width isn't specified
    {"uint8", KeywordType::UInt8},
    {"uint16", KeywordType::UInt16},
    {"uint32", KeywordType::UInt32},
    {"uint64", KeywordType::UInt64},
    {"while", KeywordType::While}};

}  // namespace core
MANG_END
