#if MN_DEBUG  // only include dump methods in debug builds

#include <core.hpp>
#include <cstddef>
#include <cstdint>
#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/number.hxx>
#include <string>
#include <string_view>
#include <utils/type_names.hpp>

namespace Manganese {
namespace ast {

// Helpers
namespace {

inline std::string getIndent(std::size_t indent) { return std::string(indent * 2, ' '); }

struct Indent {
    size_t level;
    constexpr Indent next(std::size_t delta = 1) const noexcept { return Indent{.level = level + delta}; }
    std::string str() const { return getIndent(level); }
};

inline void dumpHeader(std::ostream& os, Indent indent, std::string_view className, const ASTNode& node) {
    os << indent.str() << std::format("{} [{}:{}]\n", className, node.getLine(), node.getColumn()) << "{\n";
}

inline void dumpBlock(std::ostream& os, Indent indent, std::string_view label, const ast::Block& block) {
    os << indent.str() << label << ": [\n";
    for (const ast::Statement* stmt : block) {
        if (stmt) { stmt->dump(os, indent.next().level); }
    }
    os << indent.str() << "]\n";
}

void dumpSemanticType(std::ostream& os, Indent ind, const semantic::SemanticType* semanticType) {
    if (!semanticType) { return; }
    os << ind.str() << "semantic type: " << semanticType->toString() << "\n";
}
}  // namespace

std::string_view getNumberTypeName(const mnstl::number_t& value) {
    using enum mnstl::number_t::held_type;
    switch (value.underlying_type()) {
        case int8: return int8_str;
        case int16: return int16_str;
        case int32: return int32_str;
        case int64: return int64_str;
        case uint8: return uint8_str;
        case uint16: return uint16_str;
        case uint32: return uint32_str;
        case uint64: return uint64_str;
        case int128: return int128_str;
        case uint128: return uint128_str;
        case float32: return float32_str;
        case float64: return float64_str;
        case error: return "error";
        case none: ASSERT_UNREACHABLE("Number did not hold a value");
    }
    ASSERT_UNREACHABLE("Number did not hold a valid type");
}

// Statements

void AggregateDeclarationStatement::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "AggregateDeclarationStatement", *this);
    os << ind.next().str() << "name: " << name << "\n";
    os << ind.next().str() << "visibility: " << visibilityToString(visibility) << "\n";
    os << ind.next().str() << "fields: [\n";

    for (const auto& field : fields) {
        os << ind.next(2).str() << "{\n";
        os << ind.next(3).str() << "name: " << field.name << "\n";
        os << ind.next(3).str() << "type: \n";
        field.type->dump(os, ind.next(4).level);
        os << ind.next(2).str() << "}\n";
    }

    os << ind.next().str() << "]\n";
    os << ind.str() << "}\n";
}

void AliasStatement::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "AliasStatement", *this);
    os << ind.next().str() << "alias: " << alias << "\n";
    os << ind.next().str() << "base type: ";
    baseType->dump(os, ind.next(2).level);
    os << ind.str() << "}\n";
}

void BreakStatement::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "BreakStatement", *this);
}

void ContinueStatement::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "ContinueStatement", *this);
}

void EmptyStatement::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "EmptyStatement", *this);
}

void EnumDeclarationStatement::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "EnumDeclarationStatement", *this);

    os << ind.next().str() << "name: " << name << "\n";
    os << ind.next().str() << "visibility: " << visibilityToString(visibility) << "\n";
    os << ind.next().str() << "values: [\n";

    for (const EnumValue& val : values) {
        os << ind.next(2).str() << "{\n";
        os << ind.next(3).str() << "name: " << val.name << "\n";
        os << ind.next(3).str() << "value: " << val.value << "\n";
        os << ind.next(2).str() << "}\n";
    }

    os << ind.next().str() << "]\n";
    os << ind.str() << "}\n";
}

void ExpressionStatement::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "ExpressionStatement", *this);
    expression->dump(os, ind.next().level);
    os << ind.str() << "}\n";
}

void ForLoopStatement::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "ForLoopStatement", *this);

    auto dumpOptionalNode = [&](std::string_view label, const auto* node) {
        os << ind.next().str() << label << ": \n";
        if (node) {
            node->dump(os, ind.next(2).level);
        } else {
            os << ind.next(2).str() << "null\n";
        }
    };

    dumpOptionalNode("initializationStep", initializationStep);
    dumpOptionalNode("stopCondition", stopCondition);
    dumpOptionalNode("postExpression", postExpression);

    dumpBlock(os, ind.next(), "body", body);
    os << ind.str() << "}\n";
}

void FunctionDeclarationStatement::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "FunctionDeclarationStatement", *this);
    os << ind.next().str() << "name: " << name << "\n";
    os << ind.next().str() << "visibility: " << visibilityToString(visibility) << "\n";

    os << ind.next().str() << "generic types: [";
    for (size_t i = 0; i < genericTypes.size(); ++i) {
        os << genericTypes[i] << (i + 1 < genericTypes.size() ? ", " : "");
    }
    os << "]\n";

    os << ind.next().str() << "parameters: [\n";
    for (const auto& param : parameters) {
        os << ind.next(2).str() << "{\n";
        os << ind.next(3).str() << "name: " << param.name << "\n";
        os << ind.next(3).str() << "isMutable: " << (param.isMutable ? "true" : "false") << "\n";
        os << ind.next(3).str() << "type: \n";
        param.type->dump(os, ind.next(4).level);
        os << ind.next(2).str() << "}\n";
    }
    os << ind.next().str() << "]\n";

    os << ind.next().str() << "returnType: ";
    if (returnType) {
        os << "\n";
        returnType->dump(os, ind.next(2).level);
    } else {
        os << "void\n";
    }

    dumpBlock(os, ind.next(), "body", body);
    os << ind.str() << "}\n";
}

void IfStatement::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "IfStatement", *this);

    os << ind.next().str() << "condition: \n";
    condition->dump(os, ind.next(2).level);

    dumpBlock(os, ind.next(), "body", body);

    if (!elifs.empty()) {
        os << ind.next().str() << "elif clauses: [\n";
        for (const auto& elif : elifs) {
            os << ind.next(2).str() << "{\n";
            os << ind.next(3).str() << "condition: \n";
            elif.condition->dump(os, ind.next(4).level);
            dumpBlock(os, ind.next(3), "body", elif.body);
            os << ind.next(2).str() << "}\n";
        }
        os << ind.next().str() << "]\n";
    }

    if (!elseBody.empty()) { dumpBlock(os, ind.next(), "else body", elseBody); }

    os << ind.str() << "}\n";
}
void NestedBlockStatement::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "NestedBlockStatement", *this);
    dumpBlock(os, ind.next(), "body", block);
    os << ind.str() << "}\n";
}
void ReturnStatement::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "ReturnStatement", *this);
    if (value) {
        os << ind.next().str() << "value: \n";
        value->dump(os, ind.next(2).level);
    } else {
        os << ind.next().str() << "value: null\n";
    }
    os << ind.str() << "}\n";
}
void SwitchStatement::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "SwitchStatement", *this);

    os << ind.next().str() << "variable: \n";
    variable->dump(os, ind.next(2).level);
    os << ind.next().str() << "cases: [\n";

    for (const auto& c : cases) {
        os << ind.next(2).str() << "{\n";
        os << ind.next(3).str() << "literalValue: \n";
        c.literalValue->dump(os, ind.next(4).level);
        dumpBlock(os, ind.next(3), "body", c.body);
        os << ind.next(2).str() << "}\n";
    }

    if (!defaultBody.empty()) { dumpBlock(os, ind.next(), "default body", defaultBody); }

    os << ind.str() << "}\n";
}
void VariableDeclarationStatement::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "VariableDeclarationStatement", *this);
    os << ind.next().str() << "name: " << name << "\n";
    os << ind.next().str() << "isMutable: " << (isMutable ? "true" : "false") << "\n";
    os << ind.next().str() << "visibility: " << (visibility == Visibility::Public ? "Public" : "Private") << "\n";

    os << ind.next().str() << "value: \n";
    if (value) {
        value->dump(os, ind.next(2).level);
    } else {
        os << ind.next(2).str() << "null\n";
    }

    os << ind.next().str() << "type: \n";
    if (type) {
        type->dump(os, ind.next(2).level);
    } else {
        os << ind.next(2).str() << "auto\n";
    }

    os << ind.str() << "}\n";
}
void WhileLoopStatement::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "WhileLoopStatement", *this);
    os << ind.next().str() << "Is Do-While: " << (isDoWhile ? "true" : "false") << "\n";
    os << ind.next().str() << "condition: \n";
    condition->dump(os, ind.next(2).level);
    dumpBlock(os, ind.next(), "body", body);
    os << ind.str() << "}\n";
}

// Expressions

void AggregateInstantiationExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "AggregateInstantiationExpression", *this);
    os << ind.next().str() << "Name: " << name << "\n";
    os << ind.next().str() << "fields: [\n";
    for (const AggregateInstantiationField& field : fields) {
        os << ind.next(2).str() << "{\n";
        os << ind.next(3).str() << "name: " << field.name << "\n";
        os << ind.next(3).str() << "value: \n";
        field.value->dump(os, ind.next(4).level);
        os << ind.next(2).str() << "}\n";
    }
    os << ind.next().str() << "]\n";
    os << ind.str() << "}\n";
}

void AggregateLiteralExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "AggregateLiteralExpression", *this);
    os << ind.next().str() << "Elements {\n";
    for (const Expression* element : elements) { element->dump(os, ind.next(2).level); }
    os << ind.next().str() << "}\n";
}

void AlignofExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "AlignofExpression", *this);
    os << ind.next().str() << "type: \n";
    type->dump(os, ind.next(2).level);
    os << ind.str() << "}\n";
}

void ArrayLiteralExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "ArrayLiteralExpression", *this);

    os << ind.next().str() << "elements: [\n";

    for (const Expression* element : elements) {
        os << ind.next(2).str() << "{\n";
        element->dump(os, ind.next(3).level);
        os << ind.next(2).str() << "}\n";
    }

    os << ind.next().str() << "]\n";
    os << ind.str() << "}\n";
}

void AssignmentExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "AssignmentExpression", *this);
    os << ind.next().str() << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << ind.next().str() << "assignee: \n";
    assignee->dump(os, ind.next(2).level);
    os << ind.next().str() << "value: \n";
    value->dump(os, ind.next(2).level);
    os << ind.str() << "}\n";
}

void BinaryExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "BinaryExpression", *this);
    os << ind.next().str() << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << ind.next().str() << "left: \n";
    left->dump(os, ind.next(2).level);
    os << ind.next().str() << "right: \n";
    right->dump(os, ind.next(2).level);
    os << ind.str() << "}\n";
}

void BoolLiteralExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "BoolLiteralExpression", *this);
    os << ind.next().str() << "value: " << toString();
    os << "\n";
    os << ind.str() << "}\n";
}

void CharLiteralExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "CharLiteralExpression", *this);
    os << ind.next().str() << "value: '" << static_cast<char>(value) << "'\n";
    os << ind.next().str() << "code point: " << static_cast<std::int32_t>(value) << "\n";
    os << ind.str() << "}\n";
}

void FunctionCallExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "FunctionCallExpression", *this);
    os << ind.next().str() << "callee: \n";
    callee->dump(os, ind.next(2).level);
    os << ind.next().str() << "arguments: [\n";

    for (const Expression* arg : arguments) {
        os << ind.next(2).str() << "{\n";
        arg->dump(os, ind.next(3).level);
        os << ind.next(2).str() << "}\n";
    }

    os << ind.next().str() << "]\n";
    os << ind.str() << "}\n";
}

void GenericExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "GenericExpression", *this);
    os << ind.next().str() << "identifier: " << toStringOr(identifier) << "\n";
    os << ind.next().str() << "generic types: [\n";

    for (const Type* type : types) { os << ind.next(2).str() << toStringOr(type) << "\n"; }

    os << ind.next().str() << "]\n";
    os << ind.str() << "}\n";
}

void IdentifierExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "IdentifierExpression", *this);
    os << ind.next().str() << "name: " << value << "\n";
    os << ind.str() << "}\n";
}

void IndexExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "IndexExpression", *this);
    os << ind.next().str() << "variable: \n";
    variable->dump(os, ind.next(2).level);
    os << ind.next().str() << "index: \n";
    index->dump(os, ind.next(2).level);
    os << ind.str() << "}\n";
}

void MemberAccessExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "MemberAccessExpression", *this);
    os << ind.next().str() << "object: \n";
    object->dump(os, ind.next(2).level);
    os << ind.next().str() << "property: " << property << "\n";
    os << ind.str() << "}\n";
}

void NumberLiteralExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "NumberLiteralExpression", *this);
    os << ind.next().str() << "value: " << toString() << "\n";
    os << ind.next().str() << "Inferred literal type: " << getNumberTypeName(value) << "\n";
    os << ind.str() << "}\n";
}

void PostfixExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "PostfixExpression", *this);
    os << ind.next().str() << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << ind.next().str() << "operand: \n";
    left->dump(os, ind.next(2).level);
    os << ind.str() << "}\n";
}

void PrefixExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "PrefixExpression", *this);
    os << ind.next().str() << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << ind.next().str() << "operand: \n";
    right->dump(os, ind.next(2).level);
    os << ind.str() << "}\n";
}

void ScopeResolutionExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "ScopeResolutionExpression", *this);
    os << ind.next().str() << "scope: \n";
    scope->dump(os, ind.next(2).level);
    os << ind.next().str() << "element: " << element << "\n";
    os << ind.str() << "}\n";
}

void SizeofExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "SizeofExpression", *this);
    os << ind.next().str() << "type: \n";
    type->dump(os, ind.next(2).level);
    os << ind.str() << "}\n";
}

void StringLiteralExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "StringLiteralExpression", *this);
    os << ind.next().str() << "value: " << toString() << "\n";
    os << ind.next().str() << "length: " << value.length() << "\n";
    os << ind.str() << "}\n";
}

void TypeCastExpression::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "TypeCastExpression", *this);
    os << ind.next().str() << "expression: \n";
    originalValue->dump(os, ind.next(2).level);
    os << ind.next().str() << "target type: \n";
    targetType->dump(os, ind.next(2).level);
    os << ind.str() << "}\n";
}

// Types

void AggregateType::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "AggregateType", *this);
    os << ind.next().str() << "fields: [\n";

    for (const auto* field : fieldTypes) {
        os << ind.next(2).str() << "{\n";
        field->dump(os, ind.next(3).level);
        os << ind.next(2).str() << "}\n";
    }
    dumpSemanticType(os, ind.next(), semanticType);

    os << ind.next().str() << "]\n";
    os << ind.str() << "}\n";
}
void ArrayType::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "ArrayType", *this);
    os << ind.next().str() << "elementType:\n";
    elementType->dump(os, ind.next(2).level);

    if (lengthExpression) {
        os << ind.next().str() << "length:\n";
        lengthExpression->dump(os, ind.next(2).level);
    }
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind.str() << "}\n";
}
void FunctionType::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "FunctionType", *this);
    os << ind.next().str() << "parameter types: [\n";
    for (const auto& paramType : parameterTypes) {
        os << ind.next(2).str() << (paramType.isMutable ? "mut " : "") << "\n";
        paramType.type->dump(os, ind.next(2).level);
    }
    os << ind.next().str() << "]\n";

    os << ind.next().str() << "return type: ";
    if (returnType) {
        os << "\n";
        returnType->dump(os, ind.next(2).level);
    } else {
        os << "void\n";
    }
    dumpSemanticType(os, ind.next(), semanticType);

    os << ind.str() << "}\n";
}
void GenericType::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "GenericType", *this);

    os << ind.next().str() << "base: ";
    baseType->dump(os, ind.next().level);
    os << "\n";
    os << ind.next().str() << "generic types: [\n";

    for (const auto* type : typeParameters) { type->dump(os, ind.next(2).level); }
    dumpSemanticType(os, ind.next(), semanticType);

    os << ind.next().str() << "]\n";
    os << ind.str() << "}\n";
}
void PointerType::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "PointerType", *this);
    os << ind.next().str() << "base type: \n";
    baseType->dump(os, ind.next(2).level);
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind.str() << "}\n";
}
void SymbolType::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "SymbolType", *this);
    os << ind.next().str() << "name: " << name << "\n";
    os << ind.next().str() << "primitive type: " << primitiveTypeToString(primitiveType) << "\n";
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind.str() << "}\n";
}
void TypeofType::dump(std::ostream& os, size_t indent) const {
    Indent ind{indent};
    dumpHeader(os, ind, "TypeofType", *this);
    os << ind.next().str() << "expression: ";
    expression->dump(os, ind.next().level);
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind.str() << "}\n";
}

}  // namespace ast
}  // namespace Manganese

#endif  // MN_DEBUG