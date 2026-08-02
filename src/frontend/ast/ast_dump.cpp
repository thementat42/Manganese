#if MN_DEBUG  // only include dump methods in debug builds

#include <core.hpp>
#include <cstddef>
#include <cstdint>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/number.hxx>
#include <ostream>
#include <string>
#include <string_view>
#include <utils/type_names.hpp>

namespace Manganese::ast {

// Helpers
namespace {

struct Indent {
    const size_t level;

    constexpr Indent next(std::size_t delta = 1) const noexcept { return Indent{.level = level + delta}; }
    constexpr operator std::size_t() const noexcept { return level; }
    friend std::ostream& operator<<(std::ostream& os, Indent ind) { return os << std::string(ind.level * 2, ' '); }
};

inline void dumpHeader(std::ostream& os, Indent indent, std::string_view className, const ASTNode& node) {
    os << indent << std::format("{} [{}:{}]", className, node.getLine(), node.getColumn()) << " {\n";
}

inline void dumpBlock(std::ostream& os, Indent indent, std::string_view label, const ast::Block& block) {
    os << indent << label << ": [\n";
    for (const ast::Statement* stmt : block) {
        if (stmt) { stmt->dump(os, indent.next()); }
    }
    os << indent << "]\n";
}

void dumpSemanticType(std::ostream& os, Indent ind, const semantic::SemanticType* semanticType) {
    if (!semanticType) { return; }
    os << ind << "semantic type: " << semanticType->toString() << "\n";
}

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
}  // namespace

// Statements

void AggregateDeclarationStatement::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "AggregateDeclarationStatement", *this);
    os << ind.next() << "name: " << name << "\n";
    os << ind.next() << "visibility: " << visibilityToString(visibility) << "\n";
    os << ind.next() << "fields: [\n";

    for (const auto& field : fields) {
        os << ind.next(2) << "{\n";
        os << ind.next(3) << "name: " << field.name << "\n";
        os << ind.next(3) << "type: \n";
        field.type->dump(os, ind.next(4));
        os << ind.next(2) << "}\n";
    }

    os << ind.next() << "]\n";
    os << ind << "}\n";
}

void AliasStatement::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "AliasStatement", *this);
    os << ind.next() << "alias: " << alias << "\n";
    os << ind.next() << "base type: ";
    baseType->dump(os, ind.next(2));
    os << ind << "}\n";
}

void BreakStatement::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "BreakStatement", *this);
}

void ContinueStatement::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ContinueStatement", *this);
}

void EmptyStatement::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "EmptyStatement", *this);
}

void EnumDeclarationStatement::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "EnumDeclarationStatement", *this);

    os << ind.next() << "name: " << name << "\n";
    os << ind.next() << "visibility: " << visibilityToString(visibility) << "\n";
    os << ind.next() << "values: [\n";

    for (const EnumValue& val : values) {
        os << ind.next(2) << "{\n";
        os << ind.next(3) << "name: " << val.name << "\n";
        os << ind.next(3) << "value: " << val.value << "\n";
        os << ind.next(2) << "}\n";
    }

    os << ind.next() << "]\n";
    os << ind << "}\n";
}

void ExpressionStatement::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ExpressionStatement", *this);
    expression->dump(os, ind.next());
    os << ind << "}\n";
}

void ForLoopStatement::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ForLoopStatement", *this);

    auto dumpOptionalNode = [&](std::string_view label, const auto* node) {
        os << ind.next() << label << ": \n";
        if (node) {
            node->dump(os, ind.next(2));
        } else {
            os << ind.next(2) << "null\n";
        }
    };

    dumpOptionalNode("initializationStep", initializationStep);
    dumpOptionalNode("stopCondition", stopCondition);
    dumpOptionalNode("postExpression", postExpression);

    dumpBlock(os, ind.next(), "body", body);
    os << ind << "}\n";
}

void FunctionDeclarationStatement::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "FunctionDeclarationStatement", *this);
    os << ind.next() << "name: " << name << "\n";
    os << ind.next() << "visibility: " << visibilityToString(visibility) << "\n";

    os << ind.next() << "generic types: [";
    for (size_t i = 0; i < genericTypes.size(); ++i) {
        os << genericTypes[i] << (i + 1 < genericTypes.size() ? ", " : "");
    }
    os << "]\n";

    os << ind.next() << "parameters: [\n";
    for (const auto& param : parameters) {
        os << ind.next(2) << "{\n";
        os << ind.next(3) << "name: " << param.name << "\n";
        os << ind.next(3) << "isMutable: " << (param.isMutable ? "true" : "false") << "\n";
        os << ind.next(3) << "type: \n";
        param.type->dump(os, ind.next(4));
        os << ind.next(2) << "}\n";
    }
    os << ind.next() << "]\n";

    os << ind.next() << "returnType: ";
    if (returnType) {
        os << "\n";
        returnType->dump(os, ind.next(2));
    } else {
        os << "void\n";
    }

    dumpBlock(os, ind.next(), "body", body);
    os << ind << "}\n";
}

void IfStatement::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "IfStatement", *this);

    os << ind.next() << "condition: \n";
    condition->dump(os, ind.next(2));

    dumpBlock(os, ind.next(), "body", body);

    if (!elifs.empty()) {
        os << ind.next() << "elif clauses: [\n";
        for (const auto& elif : elifs) {
            os << ind.next(2) << "{\n";
            os << ind.next(3) << "condition: \n";
            elif.condition->dump(os, ind.next(4));
            dumpBlock(os, ind.next(3), "body", elif.body);
            os << ind.next(2) << "}\n";
        }
        os << ind.next() << "]\n";
    }

    if (!elseBody.empty()) { dumpBlock(os, ind.next(), "else body", elseBody); }

    os << ind << "}\n";
}
void NestedBlockStatement::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "NestedBlockStatement", *this);
    dumpBlock(os, ind.next(), "body", block);
    os << ind << "}\n";
}
void ReturnStatement::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ReturnStatement", *this);
    if (value) {
        os << ind.next() << "value: \n";
        value->dump(os, ind.next(2));
    } else {
        os << ind.next() << "value: null\n";
    }
    os << ind << "}\n";
}
void SwitchStatement::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "SwitchStatement", *this);

    os << ind.next() << "target: \n";
    target->dump(os, ind.next(2));
    os << ind.next() << "cases: [\n";

    for (const auto& c : cases) {
        os << ind.next(2) << "{\n";
        os << ind.next(3) << "values: [\n";
        for (const auto* val : c.values) { val->dump(os, ind.next(3)); }
        os << ind.next(3) << "]\n";
        dumpBlock(os, ind.next(3), "body", c.body);
        os << ind.next(2) << "}\n";
    }

    if (!defaultBody.empty()) { dumpBlock(os, ind.next(), "default body", defaultBody); }

    os << ind << "}\n";
}
void VariableDeclarationStatement::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "VariableDeclarationStatement", *this);
    os << ind.next() << "name: " << name << "\n";
    os << ind.next() << "isMutable: " << (isMutable ? "true" : "false") << "\n";
    os << ind.next() << "visibility: " << (visibility == Visibility::Public ? "Public" : "Private") << "\n";

    os << ind.next() << "value: \n";
    if (value) {
        value->dump(os, ind.next(2));
    } else {
        os << ind.next(2) << "null\n";
    }

    os << ind.next() << "type: \n";
    if (type) {
        type->dump(os, ind.next(2));
    } else {
        os << ind.next(2) << "auto\n";
    }

    os << ind << "}\n";
}
void WhileLoopStatement::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "WhileLoopStatement", *this);
    os << ind.next() << "Is Do-While: " << (isDoWhile ? "true" : "false") << "\n";
    os << ind.next() << "condition: \n";
    condition->dump(os, ind.next(2));
    dumpBlock(os, ind.next(), "body", body);
    os << ind << "}\n";
}

// Expressions

void AggregateInstantiationExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "AggregateInstantiationExpression", *this);
    os << ind.next() << "base:\n";
    base->dump(os, ind.next(2));

    os << ind.next() << "fields: [\n";
    for (const AggregateInstantiationField& field : fields) {
        os << ind.next(2) << "{\n";
        os << ind.next(3) << "name: " << field.name << "\n";
        os << ind.next(3) << "value: \n";
        field.value->dump(os, ind.next(4));
        os << ind.next(2) << "}\n";
    }
    os << ind.next() << "]\n";
    os << ind << "}\n";
}

void AggregateLiteralExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "AggregateLiteralExpression", *this);
    os << ind.next() << "Elements {\n";
    for (const Expression* element : elements) { element->dump(os, ind.next(2)); }
    os << ind.next() << "}\n";
}

void AlignofExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "AlignofExpression", *this);
    os << ind.next() << "type: \n";
    type->dump(os, ind.next(2));
    os << ind << "}\n";
}

void ArrayLiteralExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ArrayLiteralExpression", *this);

    os << ind.next() << "elements: [\n";

    for (const Expression* element : elements) {
        os << ind.next(2) << "{\n";
        element->dump(os, ind.next(3));
        os << ind.next(2) << "}\n";
    }

    os << ind.next() << "]\n";
    os << ind << "}\n";
}

void AssignmentExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "AssignmentExpression", *this);
    os << ind.next() << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << ind.next() << "assignee: \n";
    assignee->dump(os, ind.next(2));
    os << ind.next() << "value: \n";
    value->dump(os, ind.next(2));
    os << ind << "}\n";
}

void BinaryExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "BinaryExpression", *this);
    os << ind.next() << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << ind.next() << "left: \n";
    left->dump(os, ind.next(2));
    os << ind.next() << "right: \n";
    right->dump(os, ind.next(2));
    os << ind << "}\n";
}

void BoolLiteralExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "BoolLiteralExpression", *this);
    os << ind.next() << "value: " << toString();
    os << "\n";
    os << ind << "}\n";
}

void CharLiteralExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "CharLiteralExpression", *this);
    os << ind.next() << "value: '" << static_cast<char>(value) << "'\n";
    os << ind.next() << "code point: " << static_cast<std::int32_t>(value) << "\n";
    os << ind << "}\n";
}

void FunctionCallExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "FunctionCallExpression", *this);
    os << ind.next() << "callee: \n";
    callee->dump(os, ind.next(2));
    os << ind.next() << "arguments: [\n";

    for (const Expression* arg : arguments) {
        os << ind.next(2) << "{\n";
        arg->dump(os, ind.next(3));
        os << ind.next(2) << "}\n";
    }

    os << ind.next() << "]\n";
    os << ind << "}\n";
}

void GenericExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "GenericExpression", *this);
    os << ind.next() << "identifier: " << toStringOr(identifier) << "\n";
    os << ind.next() << "generic types: [\n";

    for (const Type* type : types) { os << ind.next(2) << toStringOr(type) << "\n"; }

    os << ind.next() << "]\n";
    os << ind << "}\n";
}

void IdentifierExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "IdentifierExpression", *this);
    os << ind.next() << "name: " << value << "\n";
    os << ind << "}\n";
}

void IndexExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "IndexExpression", *this);
    os << ind.next() << "variable: \n";
    variable->dump(os, ind.next(2));
    os << ind.next() << "index: \n";
    index->dump(os, ind.next(2));
    os << ind << "}\n";
}

void MemberAccessExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "MemberAccessExpression", *this);
    os << ind.next() << "object: \n";
    object->dump(os, ind.next(2));
    os << ind.next() << "property: " << property << "\n";
    os << ind << "}\n";
}

void NumberLiteralExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "NumberLiteralExpression", *this);
    os << ind.next() << "value: " << toString() << "\n";
    os << ind.next() << "Inferred literal type: " << getNumberTypeName(value) << "\n";
    os << ind << "}\n";
}

void PostfixExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "PostfixExpression", *this);
    os << ind.next() << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << ind.next() << "operand: \n";
    left->dump(os, ind.next(2));
    os << ind << "}\n";
}

void PrefixExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "PrefixExpression", *this);
    os << ind.next() << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << ind.next() << "operand: \n";
    right->dump(os, ind.next(2));
    os << ind << "}\n";
}

void ScopeResolutionExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ScopeResolutionExpression", *this);
    os << ind.next() << "scope: \n";
    scope->dump(os, ind.next(2));
    os << ind.next() << "element: \n";
    element->dump(os, ind.next(2));
}

void SizeofExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "SizeofExpression", *this);
    os << ind.next() << "type: \n";
    type->dump(os, ind.next(2));
    os << ind << "}\n";
}

void StringLiteralExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "StringLiteralExpression", *this);
    os << ind.next() << "value: " << toString() << "\n";
    os << ind.next() << "length: " << value.length() << "\n";
    os << ind << "}\n";
}

void TypeCastExpression::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "TypeCastExpression", *this);
    os << ind.next() << "expression: \n";
    originalValue->dump(os, ind.next(2));
    os << ind.next() << "target type: \n";
    targetType->dump(os, ind.next(2));
    os << ind << "}\n";
}

// Types

void AggregateType::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "AggregateType", *this);
    os << ind.next() << "fields: [\n";

    for (const auto* field : fieldTypes) {
        os << ind.next(2) << "{\n";
        field->dump(os, ind.next(3));
        os << ind.next(2) << "}\n";
    }
    dumpSemanticType(os, ind.next(), semanticType);

    os << ind.next() << "]\n";
    os << ind << "}\n";
}
void ArrayType::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ArrayType", *this);
    os << ind.next() << "elementType:\n";
    elementType->dump(os, ind.next(2));

    if (lengthExpression) {
        os << ind.next() << "length:\n";
        lengthExpression->dump(os, ind.next(2));
    }
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}
void FunctionType::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "FunctionType", *this);
    os << ind.next() << "parameter types: [\n";
    for (const auto& paramType : parameterTypes) {
        os << ind.next(2) << (paramType.isMutable ? "mut " : "") << "\n";
        paramType.type->dump(os, ind.next(2));
    }
    os << ind.next() << "]\n";

    os << ind.next() << "return type: ";
    if (returnType) {
        os << "\n";
        returnType->dump(os, ind.next(2));
    } else {
        os << "void\n";
    }
    dumpSemanticType(os, ind.next(), semanticType);

    os << ind << "}\n";
}
void GenericType::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "GenericType", *this);

    os << ind.next() << "base: ";
    baseType->dump(os, ind.next());
    os << "\n";
    os << ind.next() << "generic types: [\n";

    for (const auto* type : typeParameters) { type->dump(os, ind.next(2)); }
    dumpSemanticType(os, ind.next(), semanticType);

    os << ind.next() << "]\n";
    os << ind << "}\n";
}
void PointerType::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "PointerType", *this);
    os << ind.next() << "base type: \n";
    baseType->dump(os, ind.next(2));
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void ScopedType::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ScopedType", *this);
    os << ind.next() << "qualifier: \n";
    qualifier->dump(os, ind.next(2));
    os << ind.next() << "base type: \n";
    baseType->dump(os, ind.next(2));
    os << ind << "}\n";
}

void SymbolType::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "SymbolType", *this);
    os << ind.next() << "name: " << name << "\n";
    os << ind.next() << "primitive type: " << primitiveTypeToString(primitiveType) << "\n";
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}
void TypeofType::dump(std::ostream& os, size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "TypeofType", *this);
    os << ind.next() << "expression: ";
    expression->dump(os, ind.next());
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

}  // namespace Manganese::ast

#endif  // MN_DEBUG