#include <concepts>
#include <cstddef>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/semantic/type_context.hpp>
#include <string>
#include <utils/string_interner.hpp>
#include <vector>

#if MN_DEBUG
#define WRAP(str) "(" str ")"
#else
#define WRAP(str) str
#endif  // MN_DEBUG

namespace Manganese::ast {

// Helpers
namespace {

inline std::string getIndent(std::size_t indent) { return std::string(indent * 4, ' '); }

std::string blockToString(const Block& block, const utils::StringInterner& interner, std::size_t indent) {
    std::string result = "{\n";
    for (const Statement* stmt : block) { result += stmt->toString(interner, indent + 1) + "\n"; }
    result += getIndent(indent) + "}";
    return result;
}

template <class T>
    requires(std::derived_from<T, ASTNode>)
std::string commaSeparatedList(const mnstl::Slice<T*>& values, const utils::StringInterner& interner,
                               std::size_t indent = 0) {
    std::string result;
    for (std::size_t i = 0; i < values.size; ++i) {
        result += values[i]->toString(interner, indent);
        if (i != values.size - 1) [[likely]] { result += ", "; }
    }
    return result;
}

template <class T>
    requires(std::same_as<T, utils::StringID>)
std::string commaSeparatedList(const mnstl::Slice<T>& values, const utils::StringInterner& interner) {
    std::string result;
    for (std::size_t i = 0; i < values.size; ++i) {
        result += interner.get_view(values[i]);
        if (i != values.size - 1) [[likely]] { result += ", "; }
    }
    return result;
}

std::string genericsToString(const mnstl::Slice<Type*>& params, const utils::StringInterner& interner,
                             std::size_t indent = 0) {
    if (params.empty()) { return ""; }
    return std::format("@[{}]", commaSeparatedList(params, interner, indent));
}

}  // namespace

// Statements

std::string AggregateDeclarationStatement::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string result
        = getIndent(indent) + std::format("{} aggregate {}", visibilityToString(visibility), interner.get_view(name));
    if (!genericTypes.empty()) { result += std::format("[{}]", commaSeparatedList(genericTypes, interner)); }
    result += " {\n";
    for (const AggregateField& field : fields) {
        result += getIndent(indent + 1) + std::string(interner.get_view(field.name)) + ": "
            + field.type->toString(interner, indent + 1) + ";\n";
    }
    result += getIndent(indent) + "}";
    return result;
}

std::string AliasStatement::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return getIndent(indent)
        + std::format("alias {} = " WRAP("{}") ";", interner.get_view(name), baseType->toString(interner, indent));
}

std::string BreakStatement::toString(const utils::StringInterner& /*unused*/, std::size_t indent) const {
    return getIndent(indent) + "break;";
}

std::string ContinueStatement::toString(const utils::StringInterner& /*unused*/, std::size_t indent) const {
    return getIndent(indent) + "continue;";
}

std::string EmptyStatement::toString(const utils::StringInterner& /*unused*/, std::size_t /*indent*/) const {
    return "";
}

std::string EnumDeclarationStatement::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string baseTypeStr = (baseType != nullptr) ? std::format(": {}", baseType->toString(interner, indent)) : "";
    std::string result = getIndent(indent)
        + std::format("{} enum {}{}", visibilityToString(visibility), interner.get_view(name), baseTypeStr);
    result += " {\n";
    for (std::size_t i = 0; i < values.size; ++i) {
        const EnumValue& value = values[i];
        result += getIndent(indent + 1) + std::string(interner.get_view(value.name));
        if (value.value != nullptr) { result += std::format(" = {}", value.value->toString(interner, indent + 1)); }
        if (i != values.size - 1) { result += ","; }
        result += '\n';
    }
    result += getIndent(indent) + "}";
    return result;
}

std::string ExpressionStatement::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return getIndent(indent) + std::format("{};", expression->toString(interner, indent));
}

std::string ForLoopStatement::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string result = getIndent(indent) + "for (";
    if (initializationStep != nullptr) {
        result += initializationStep->toString(interner, 0) + " ";
    } else {
        result += ";";
    }
    if (stopCondition != nullptr) {
        result += stopCondition->toString(interner, 0) + "; ";
    } else {
        result += ";";
    }
    if (postExpression != nullptr) { result += postExpression->toString(interner, 0); }
    result += ") ";
    result += blockToString(body, interner, indent);
    return result;
}

std::string FunctionDeclarationStatement::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string result
        = getIndent(indent) + std::format("{} func {}", visibilityToString(visibility), interner.get_view(name));

    if (!genericTypes.empty()) { result += std::format("[{}]", commaSeparatedList(genericTypes, interner)); }

    result += '(';
    for (std::size_t i = 0; i < parameters.size; ++i) {
        const FunctionParameter& param = parameters[i];
        result += interner.get_view(param.name);
        if (param.isVariadic) { result += "..."; }
        result += std::format(": {}{}", (param.isMutable ? "mut " : ""), param.type->toString(interner, 0));

        if (param.defaultValue != nullptr) {
            result += std::format(" = {}", param.defaultValue->toString(interner, 0));
        }

        if (i < parameters.size - 1) { result += ", "; }
    }
    result += ')';
    if (returnType != nullptr) { result += std::format(" -> {}", returnType->toString(interner, indent)); }
    result += ' ';
    result += blockToString(body, interner, indent);
    return result;
}

std::string IfStatement::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string result = getIndent(indent) + std::format("if ({}) ", condition->toString(interner, indent))
        + blockToString(body, interner, indent);
    for (const ElifClause& elif : elifs) {
        result += std::format(" elif ({}) ", elif.condition->toString(interner, indent))
            + blockToString(elif.body, interner, indent);
    }
    if (elseBody.has_value()) { result += " else " + blockToString(*elseBody, interner, indent); }
    return result;
}

std::string ImportStatement::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string pathStr;
    for (std::size_t i = 0; i < path.size; ++i) {
        pathStr += interner.get_view(path[i]);
        if (i != path.size - 1) [[likely]] { pathStr += "::"; }
    }
    const std::string aliasStr = alias.has_value() ? std::format("as {}", interner.get_view(*alias)) : "";

    return getIndent(indent) + std::format("import {} {};", pathStr, aliasStr);
}

std::string NamespaceStatement::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return getIndent(indent) + std::format("namespace {} ", interner.get_view(name))
        + blockToString(block, interner, indent);
}

std::string ModuleDeclarationStatement::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return getIndent(indent) + std::format("module {};", interner.get_view(name));
}

std::string NestedBlockStatement::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return getIndent(indent) + blockToString(block, interner, indent);
}

std::string ReturnStatement::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string valStr = (value != nullptr) ? value->toString(interner, indent) : "";
    return getIndent(indent) + std::format("return {};", valStr);
}

std::string SwitchStatement::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string result = getIndent(indent) + std::format("switch ({})", target->toString(interner, indent)) + " {\n";
    for (const CaseClause& _case : cases) {
        result += getIndent(indent + 1) + std::format("case {}:\n", commaSeparatedList(_case.values, interner));
        for (const Statement* stmt : _case.body) { result += stmt->toString(interner, indent + 2) + "\n"; }
    }
    if (defaultBody.has_value()) {
        result += getIndent(indent + 1) + "default:\n";
        for (const Statement* stmt : *defaultBody) { result += stmt->toString(interner, indent + 2) + "\n"; }
    }
    result += getIndent(indent) + "}";
    return result;
}

std::string VariableDeclarationStatement::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string typeName;

    if (type == nullptr) {
        typeName = "auto";
    } else if (type->semanticType != nullptr) {
        typeName = type->semanticType->toString();
    } else if ((value != nullptr) && (value->semanticType != nullptr)) {
        typeName = value->semanticType->toString();
    } else {
        typeName = type->toString(interner, 0);
    }
    std::string typeStr = std::format("{} {}", visibilityToString(visibility), typeName);
    std::string valueStr = (value == nullptr) ? "" : " = " + value->toString(interner, 0);

    return getIndent(indent)
        + std::format("({} {}: {}{});", isMutable ? "let mut" : "let", interner.get_view(name), typeStr, valueStr);
}

std::string WhileLoopStatement::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string result = getIndent(indent);
    const std::string whileCond = std::format("while ({})", condition->toString(interner, indent));
    if (isDoWhile) {
        result += "do ";
    } else {
        result += whileCond + ' ';
    }
    result += blockToString(body, interner, indent);
    if (isDoWhile) { result += " " + whileCond + ";"; }

    return result;
}

// Expressions

std::string AggregateInstantiationExpression::toString(const utils::StringInterner& interner,
                                                       std::size_t indent) const {
    std::string result = base->toString(interner, indent) + " {";
    for (std::size_t i = 0; i < fields.size; ++i) {
        const AggregateInstantiationField& field = fields[i];
        result += std::format("{} = {}", interner.get_view(field.name), field.value->toString(interner, indent));
        if (i != fields.size - 1) { result += ", "; }
    }
    result += "}";
    return result;
}

std::string AggregateLiteralExpression::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string result = "{";
    for (std::size_t i = 0; i < elements.size; ++i) {
        result += elements[i]->toString(interner, indent);
        if (i < elements.size - 1) [[likely]] { result += ", "; }
    }
    result += "}";
    return result;
}

std::string AlignofExpression::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return std::format(WRAP("alignof({})"), type->toString(interner, indent));
}

std::string ArrayLiteralExpression::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string result = "[";
    for (std::size_t i = 0; i < elements.size; ++i) {
        result += elements[i]->toString(interner, indent);
        if (i < elements.size - 1) [[likely]] { result += ", "; }
    }
    result += "]";
    return result;
}

std::string AssignmentExpression::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return std::format(WRAP("{} {} {}"), assignee->toString(interner, indent), lexer::tokenTypeToString(op),
                       value->toString(interner, indent));
}
std::string BinaryExpression::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return std::format(WRAP("{} {} {}"), left->toString(interner, indent), lexer::tokenTypeToString(op),
                       right->toString(interner, indent));
}

std::string BoolLiteralExpression::toString(const utils::StringInterner& /*unused*/, std::size_t /*indent*/) const {
    return value ? "true" : "false";
}

std::string CharLiteralExpression::toString(const utils::StringInterner& /*unused*/, std::size_t /*indent*/) const {
    return std::format("'{}'", lexer::codepointToUTF8(value));
}

std::string FunctionCallExpression::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string result = callee->toString(interner, indent) + "(";
    for (std::size_t i = 0; i < arguments.size; ++i) {
        result += arguments[i]->toString(interner, indent);
        if (i < arguments.size - 1) [[likely]] { result += ", "; }
    }
    result += ")";
    return result;
}

std::string GenericInstantiationExpression::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return identifier->toString(interner, indent) + genericsToString(types, interner, indent);
}

std::string IdentifierExpression::toString(const utils::StringInterner& interner, std::size_t /*indent*/) const {
    return std::string(interner.get_view(name));
}

std::string IndexExpression::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return std::format("{}[{}]", variable->toString(interner, indent), index->toString(interner, indent));
}

std::string MemberAccessExpression::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return std::format("{}.{}", object->toString(interner, indent), interner.get_view(field));
}

std::string NumberLiteralExpression::toString(const utils::StringInterner& interner, std::size_t /*indent*/) const {
    return interner.get_copy(value);
}

std::string PostfixExpression::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return std::format(WRAP("{}{}"), left->toString(interner, indent), lexer::tokenTypeToString(op));
}

std::string PrefixExpression::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return std::format(WRAP("{}{}"), lexer::tokenTypeToString(op), right->toString(interner, indent));
}

std::string ScopeResolutionExpression::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return std::format("{}::{}", scope->toString(interner, indent), element->toString(interner, 0));
}

std::string SizeofExpression::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return std::format(WRAP("sizeof({})"), type->toString(interner, indent));
}

std::string StringLiteralExpression::toString(const utils::StringInterner& interner, std::size_t /*indent*/) const {
    return std::format("\"{}\"", interner.get_view(value));
}

std::string TernaryExpression::toString(const utils::StringInterner& interner, std::size_t /*indent*/) const {
    return std::format(WRAP("{} ? {} : {}"), condition->toString(interner, 0), ifTrue->toString(interner, 0),
                       ifFalse->toString(interner, 0));
}

std::string TypeCastExpression::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return std::format(WRAP("{} as {}"), originalValue->toString(interner, indent),
                       targetType->toString(interner, indent));
}

std::string UninitializedExpression::toString(const utils::StringInterner& /*unused*/, std::size_t /*indent*/) const {
    return std::format(WRAP("uninitialized"));
}

// Types

std::string AggregateType::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string result = "aggregate {";
    for (std::size_t i = 0; i < fieldTypes.size; ++i) {
        result += fieldTypes[i]->toString(interner, indent);
        if (i < fieldTypes.size - 1) [[likely]] { result += ", "; }
    }
    result += "}";
    return result;
}

std::string ArrayType::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string lengthStr;
    if (lengthExpression != nullptr) {
        lengthStr = lengthExpression->toString(interner, 0);
    } else if ((semanticType != nullptr) && semanticType->isArray()) {
        const auto* arrayType = static_cast<const semantic::Array*>(semanticType);
        lengthStr = arrayType->hasUnspecifiedLength() ? "" : std::to_string(*arrayType->length);
    }
    return std::format("{}[{}]", elementType->toString(interner, indent), lengthStr);
}

std::string FunctionType::toString(const utils::StringInterner& interner, std::size_t indent) const {
    std::string result = "func(";
    for (std::size_t i = 0; i < parameterTypes.size; ++i) {
        const FunctionParameterType& param = parameterTypes[i];
        result += std::format("{}{}", (param.isMutable ? "mut " : ""), param.type->toString(interner, indent));
        if (i != parameterTypes.size - 1) { result += ", "; }
    }
    result += ")";
    if (returnType != nullptr) { result += std::format(" -> {}", returnType->toString(interner, indent)); }
    return result;
}

std::string GenericInstantiationType::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return baseType->toString(interner, indent) + genericsToString(typeParameters, interner, indent);
}

std::string IdentifierType::toString(const utils::StringInterner& interner, std::size_t /*indent*/) const {
    return std::string(interner.get_view(name));
}

std::string PointerType::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return std::format("ptr {}{}", (isMutable ? "mut " : ""), baseType->toString(interner, indent));
}

std::string ScopedType::toString(const utils::StringInterner& interner, std::size_t /*indent*/) const {
    return std::format("{}::{}", scope->toString(interner, 0), type->toString(interner, 0));
}

std::string TypeofType::toString(const utils::StringInterner& interner, std::size_t indent) const {
    return std::format("typeof({})", expression->toString(interner, indent));
}

// Errors
std::string PoisonedStatement::toString(const utils::StringInterner& /*unused*/, std::size_t /*indent*/) const {
    return std::format("<invalid expression>");
}
std::string PoisonedExpression::toString(const utils::StringInterner& /*unused*/, std::size_t /*indent*/) const {
    return std::format("<invalid statement>");
}
std::string PoisonedType::toString(const utils::StringInterner& /*unused*/, std::size_t /*indent*/) const {
    return std::format("<invalid type>");
}

}  // namespace Manganese::ast