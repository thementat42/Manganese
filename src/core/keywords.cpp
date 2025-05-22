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
    auto it = keyword_map.find(keyword);
    if (it != keyword_map.end()) {
        return it->second;
    }

    return std::nullopt;
}

DEBUG_FUNC std::string keywordToString(KeywordType keyword) {
#if DEBUG
    switch (keyword) {
        case KeywordType::Alias:
            return "KeywordType::Alias";
        case KeywordType::As:
            return "KeywordType::As";
        case KeywordType::Blueprint:
            return "KeywordType::Blueprint";
        case KeywordType::Bool:
            return "KeywordType::Bool";
        case KeywordType::Break:
            return "KeywordType::Break";
        case KeywordType::Bundle:
            return "KeywordType::Bundle";
        case KeywordType::Case:
            return "KeywordType::Case";
        case KeywordType::Cast:
            return "KeywordType::Cast";
        case KeywordType::Char:
            return "KeywordType::Char";
        case KeywordType::Const:
            return "KeywordType::Const";
        case KeywordType::Continue:
            return "KeywordType::Continue";
        case KeywordType::Default:
            return "KeywordType::Default";
        case KeywordType::Do:
            return "KeywordType::Do";
        case KeywordType::Elif:
            return "KeywordType::Elif";
        case KeywordType::Else:
            return "KeywordType::Else";
        case KeywordType::Enum:
            return "KeywordType::Enum";
        case KeywordType::False:
            return "KeywordType::False";
        case KeywordType::Float:
            return "KeywordType::Float";
        case KeywordType::Float32:
            return "KeywordType::Float32";
        case KeywordType::Float64:
            return "KeywordType::Float64";
        case KeywordType::For:
            return "KeywordType::For";
        case KeywordType::Func:
            return "KeywordType::Func";
        case KeywordType::If:
            return "KeywordType::If";
        case KeywordType::Import:
            return "KeywordType::Import";
        case KeywordType::Int8:
            return "KeywordType::Int8";
        case KeywordType::Int16:
            return "KeywordType::Int16";
        case KeywordType::Int32:
            return "KeywordType::Int32";
        case KeywordType::Int64:
            return "KeywordType::Int64";
        case KeywordType::Lambda:
            return "KeywordType::Lambda";
        case KeywordType::Module:
            return "KeywordType::Module";
        case KeywordType::Ptr:
            return "KeywordType::Ptr";
        case KeywordType::Public:
            return "KeywordType::Public";
        case KeywordType::ReadOnly:
            return "KeywordType::ReadOnly";
        case KeywordType::Repeat:
            return "KeywordType::Repeat";
        case KeywordType::Return:
            return "KeywordType::Return";
        case KeywordType::Switch:
            return "KeywordType::Switch";
        case KeywordType::True:
            return "KeywordType::True";
        case KeywordType::TypeOf:
            return "KeywordType::TypeOf";
        case KeywordType::UInt8:
            return "KeywordType::UInt8";
        case KeywordType::UInt16:
            return "KeywordType::UInt16";
        case KeywordType::UInt32:
            return "KeywordType::UInt32";
        case KeywordType::UInt64:
            return "KeywordType::UInt64";
        case KeywordType::While:
            return "KeywordType::While";
        default:
            return "Unknown Keyword";
    }
#else  // ^ DEBUG ^ | v !DEBUG v
    return "";
#endif // DEBUG
}

std::unordered_map<std::string, const KeywordType> keyword_map = {
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
