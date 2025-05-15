/**
 * @file keywords.cpp
 * @brief This file contains the implementation of the keyword functions for the Manganese compiler.
 */

#include "include/keywords.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "../../global_macros.h"

MANG_BEGIN
namespace lexer {

std::optional<KeywordType> keywordFromString(const std::string& keyword) {
    auto it = keyword_map.find(keyword);
    if (it != keyword_map.end()) {
        return it->second;
    }

    return std::nullopt;
}

std::string keywordToString(KeywordType keyword) {
#if DEBUG
    switch (keyword) {
        case KeywordType::ALIAS:
            return "KeywordType::ALIAS";
        case KeywordType::ARR:
            return "KeywordType::ARR";
        case KeywordType::AS:
            return "KeywordType::AS";
        case KeywordType::BLUEPRINT:
            return "KeywordType::BLUEPRINT";
        case KeywordType::BOOL:
            return "KeywordType::BOOL";
        case KeywordType::BREAK:
            return "KeywordType::BREAK";
        case KeywordType::BUNDLE:
            return "KeywordType::BUNDLE";
        case KeywordType::CASE:
            return "KeywordType::CASE";
        case KeywordType::CAST:
            return "KeywordType::CAST";
        case KeywordType::CHAR:
            return "KeywordType::CHAR";
        case KeywordType::CONST:
            return "KeywordType::CONST";
        case KeywordType::CONTINUE:
            return "KeywordType::CONTINUE";
        case KeywordType::DEFAULT:
            return "KeywordType::DEFAULT";
        case KeywordType::DO:
            return "KeywordType::DO";
        case KeywordType::ELIF:
            return "KeywordType::ELIF";
        case KeywordType::ELSE:
            return "KeywordType::ELSE";
        case KeywordType::ENUM:
            return "KeywordType::ENUM";
        case KeywordType::FALSE:
            return "KeywordType::FALSE";
        case KeywordType::FLOAT:
            return "KeywordType::FLOAT";
        case KeywordType::FOR:
            return "KeywordType::FOR";
        case KeywordType::FUNC:
            return "KeywordType::FUNC";
        case KeywordType::GARBAGE:
            return "KeywordType::GARBAGE";
        case KeywordType::IF:
            return "KeywordType::IF";
        case KeywordType::IMPORT:
            return "KeywordType::IMPORT";
        case KeywordType::INT8:
            return "KeywordType::INT8";
        case KeywordType::INT16:
            return "KeywordType::INT16";
        case KeywordType::INT32:
            return "KeywordType::INT32";
        case KeywordType::INT64:
            return "KeywordType::INT64";
        case KeywordType::LAMBDA:
            return "KeywordType::LAMBDA";
        case KeywordType::MAP:
            return "KeywordType::MAP";
        case KeywordType::MATCH:
            return "KeywordType::MATCH";
        case KeywordType::MODULE:
            return "KeywordType::MODULE";
        case KeywordType::PTR:
            return "KeywordType::PTR";
        case KeywordType::PRIVATE:
            return "KeywordType::PRIVATE";
        case KeywordType::PUBLIC:
            return "KeywordType::PUBLIC";
        case KeywordType::READONLY:
            return "KeywordType::READONLY";
        case KeywordType::REPEAT:
            return "KeywordType::REPEAT";
        case KeywordType::RETURN:
            return "KeywordType::RETURN";
        case KeywordType::SET:
            return "KeywordType::SET";
        case KeywordType::STRING:
            return "KeywordType::STR";
        case KeywordType::SWITCH:
            return "KeywordType::SWITCH";
        case KeywordType::TRUE:
            return "KeywordType::TRUE";
        case KeywordType::TYPEOF:
            return "KeywordType::TYPEOF";
        case KeywordType::UNSIGNED:
            return "KeywordType::UNSIGNED";
        case KeywordType::VARIANT:
            return "KeywordType::VARIANT";
        case KeywordType::VEC:
            return "KeywordType::VEC";
        case KeywordType::WHILE:
            return "KeywordType::WHILE";
        default:
            return "Unknown Keyword";
    }
#else  // ^ DEBUG ^ | v !DEBUG v
    return "";
#endif // DEBUG
}

inline std::unordered_map<std::string, const KeywordType> keyword_map = {
    {"alias", KeywordType::ALIAS},
    {"arr", KeywordType::ARR},
    {"as", KeywordType::AS},
    {"blueprint", KeywordType::BLUEPRINT},
    {"bool", KeywordType::BOOL},
    {"break", KeywordType::BREAK},
    {"bundle", KeywordType::BUNDLE},
    {"case", KeywordType::CASE},
    {"cast", KeywordType::CAST},
    {"char", KeywordType::CHAR},
    {"const", KeywordType::CONST},
    {"continue", KeywordType::CONTINUE},
    {"default", KeywordType::DEFAULT},
    {"do", KeywordType::DO},
    {"elif", KeywordType::ELIF},
    {"else", KeywordType::ELSE},
    {"enum", KeywordType::ENUM},
    {"false", KeywordType::FALSE},
    {"float", KeywordType::FLOAT},  // default to float32 when floating point width isn't specified
    {"float32", KeywordType::FLOAT32},
    {"float64", KeywordType::FLOAT64},
    {"for", KeywordType::FOR},
    {"func", KeywordType::FUNC},
    {"garbage", KeywordType::GARBAGE},
    {"if", KeywordType::IF},
    {"import", KeywordType::IMPORT},
    {"int", KeywordType::INT32},  // default to int32 when integer width isn't specified
    {"int16", KeywordType::INT16},
    {"int32", KeywordType::INT32},
    {"int64", KeywordType::INT64},
    {"int8", KeywordType::INT8},
    {"lambda", KeywordType::LAMBDA},
    {"map", KeywordType::MAP},
    {"match", KeywordType::MATCH},
    {"module", KeywordType::MODULE},
    {"ptr", KeywordType::PTR},
    {"private", KeywordType::PRIVATE},
    {"public", KeywordType::PUBLIC},
    {"readonly", KeywordType::READONLY},
    {"repeat", KeywordType::REPEAT},
    {"return", KeywordType::RETURN},
    {"set", KeywordType::SET},
    {"string", KeywordType::STRING},
    {"switch", KeywordType::SWITCH},
    {"true", KeywordType::TRUE},
    {"typeof", KeywordType::TYPEOF},
    {"unsigned", KeywordType::UNSIGNED},
    {"variant", KeywordType::VARIANT},
    {"vec", KeywordType::VEC},
    {"while", KeywordType::WHILE}};

}  // namespace lexer
MANG_END
