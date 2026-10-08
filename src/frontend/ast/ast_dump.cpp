#if MN_DEBUG  // only include dump methods in debug builds

#include <core.hpp>
#include <cstddef>
#include <cstdint>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/semantic/type_context.hpp>
#include <ostream>
#include <string>
#include <string_view>
#include <utils/type_names.hpp>

namespace Manganese::ast {

// Helpers
namespace {

struct Indent {
    const std::size_t level;

    Indent next(std::size_t delta = 1) const noexcept { return Indent{.level = level + delta}; }
    operator std::size_t() const noexcept { return level; }
    friend std::ostream& operator<<(std::ostream& os, Indent ind) { return os << std::string(ind.level * 2, ' '); }
};

inline void dumpHeader(std::ostream& os, Indent indent, std::string_view className, const ASTNode& node) {
    os << indent << std::format("{} [{}:{}]", className, node.line, node.column) << " {\n";
}

inline void dumpBlock(std::ostream& os, std::string_view label, const utils::StringInterner& interner, Indent indent, const ast::Block& block) {
    os << indent << label << ": [\n";
    for (const ast::Statement* stmt : block) { stmt->dump(os, interner, indent.next()); }
    os << indent << "]\n";
}

void dumpSemanticType(std::ostream& os, Indent ind, const semantic::SemanticType* semanticType) {
    os << ind << "semantic type: ";
    if (semanticType != nullptr) {
        os << semanticType->toString();
    } else {
        os << "not yet deduced";
    }
    os << "\n";
}

inline std::size_t utf8Length(std::string_view str) noexcept {
    std::size_t count = 0;
    for (const char c : str) {
        if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) { ++count; }
    }
    return count;
}

}  // namespace

// Statements

void AggregateDeclarationStatement::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "AggregateDeclarationStatement", *this);
    os << ind.next() << "name: " << interner.get_view(name) << "\n";
    os << ind.next() << "visibility: " << visibilityToString(visibility) << "\n";
    os << ind.next() << "fields: [\n";

    for (const auto& field : fields) {
        os << ind.next(2) << "{\n";
        os << ind.next(3) << "name: " << interner.get_view(field.name) << "\n";
        os << ind.next(3) << "type: \n";
        field.type->dump(os, interner, ind.next(4));
        os << ind.next(2) << "}\n";
    }

    os << ind.next() << "]\n";
    os << ind << "}\n";
}

void AliasStatement::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "AliasStatement", *this);
    os << ind.next() << "alias: " << interner.get_view(name) << "\n";
    os << ind.next() << "base type: ";
    baseType->dump(os, interner, ind.next(2));
    os << ind << "}\n";
}

void BreakStatement::dump(std::ostream& os, const utils::StringInterner& /*unused*/, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "BreakStatement", *this);
}

void ContinueStatement::dump(std::ostream& os, const utils::StringInterner&  /*unused*/, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ContinueStatement", *this);
}

void EmptyStatement::dump(std::ostream& os, const utils::StringInterner&  /*unused*/, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "EmptyStatement", *this);
    os << " }";  // header includes an opening curly brace
}

void EnumDeclarationStatement::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "EnumDeclarationStatement", *this);

    os << ind.next() << "name: " << interner.get_view(name) << "\n";
    os << ind.next() << "visibility: " << visibilityToString(visibility) << "\n";
    os << ind.next() << "values: [\n";

    for (const EnumValue& val : values) {
        os << ind.next(2) << "{\n";
        os << ind.next(3) << "name: " << interner.get_view(val.name) << "\n";
        os << ind.next(3) << "value: \n";
        val.value->dump(os, interner, ind.next(4));
        os << ind.next(2) << "}\n";
    }

    os << ind.next() << "]\n";
    os << ind << "}\n";
}

void ExpressionStatement::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ExpressionStatement", *this);
    expression->dump(os, interner, ind.next());
    os << ind << "}\n";
}

void ForLoopStatement::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ForLoopStatement", *this);

    auto dumpOptionalNode = [&](std::string_view label, const auto* node) {
        os << ind.next() << label << ": \n";
        if (node) {
            node->dump(os, interner, ind.next(2));
        } else {
            os << ind.next(2) << "null\n";
        }
    };

    dumpOptionalNode("initializationStep", initializationStep);
    dumpOptionalNode("stopCondition", stopCondition);
    dumpOptionalNode("postExpression", postExpression);

    dumpBlock(os, "body", interner, ind.next(), body);
    os << ind << "}\n";
}

void FunctionDeclarationStatement::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "FunctionDeclarationStatement", *this);
    os << ind.next() << "name: " << interner.get_view(name) << "\n";
    os << ind.next() << "visibility: " << visibilityToString(visibility) << "\n";

    os << ind.next() << "generic types: [";
    for (std::size_t i = 0; i < genericTypes.size(); ++i) {
        os << interner.get_view(genericTypes[i]) << (i + 1 < genericTypes.size() ? ", " : "");
    }
    os << "]\n";

    os << ind.next() << "parameters: [\n";
    for (const auto& param : parameters) {
        os << ind.next(2) << "{\n";
        os << ind.next(3) << "name: " << interner.get_view(param.name) << "\n";
        os << ind.next(3) << "isMutable: " << (param.isMutable ? "true" : "false") << "\n";
        os << ind.next(3) << "isVariadic: " << (param.isVariadic ? "true" : "false") << "\n";
        os << ind.next(3) << "type: \n";
        param.type->dump(os, interner, ind.next(4));

        os << ind.next(3) << "default value: ";
        if (param.defaultValue != nullptr) {
            os << "\n";
            param.defaultValue->dump(os, interner, ind.next(4));
        } else {
            os << "none\n";
        }
        os << ind.next(2) << "}\n";
    }
    os << ind.next() << "]\n";

    os << ind.next() << "returnType: ";
    if (returnType != nullptr) {
        os << "\n";
        returnType->dump(os, interner, ind.next(2));
    } else {
        os << "void\n";
    }

    dumpBlock(os, "body", interner, ind.next(), body);
    os << ind << "}\n";
}

void IfStatement::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "IfStatement", *this);

    os << ind.next() << "condition: \n";
    condition->dump(os, interner, ind.next(2));

    dumpBlock(os, "body", interner, ind.next(), body);

    if (!elifs.empty()) {
        os << ind.next() << "elif clauses: [\n";
        for (const auto& elif : elifs) {
            os << ind.next(2) << "{\n";
            os << ind.next(3) << "condition: \n";
            elif.condition->dump(os, interner, ind.next(4));
            dumpBlock(os, "body", interner, ind.next(3), elif.body);
            os << ind.next(2) << "}\n";
        }
        os << ind.next() << "]\n";
    }

    if (!elseBody.empty()) { dumpBlock(os, "else body", interner, ind.next(), elseBody); }

    os << ind << "}\n";
}

void ImportStatement::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ImportStatement", *this);

    os << ind.next() << "path: [";
    for (std::size_t i = 0; i < path.size(); ++i) {
        os << interner.get_view(path[i]);
        if (i + 1 < path.size()) { os << ", "; }
    }
    os << "]\n";

    os << ind.next() << std::format("alias: {}\n", alias.has_value() ? std::string(interner.get_view(*alias)) : "none");
    os << ind << "}\n";
}

void ModuleDeclarationStatement::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ModuleDeclarationStatement", *this);
    os << ind.next() << "module name: " << interner.get_view(name) << "\n";
    os << ind << "}\n";
}

void NamespaceStatement::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "NamespaceStatement", *this);
    os << ind.next() << "name: " << interner.get_view(name) << "\n";
    dumpBlock(os, "statement", interner, ind.next(), block);
    os << ind << "}\n";
}

void NestedBlockStatement::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "NestedBlockStatement", *this);
    dumpBlock(os, "body", interner, ind.next(), block);
    os << ind << "}\n";
}

void ReturnStatement::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ReturnStatement", *this);
    if (value != nullptr) {
        os << ind.next() << "value: \n";
        value->dump(os, interner, ind.next(2));
    } else {
        os << ind.next() << "value: null\n";
    }
    os << ind << "}\n";
}

void SwitchStatement::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "SwitchStatement", *this);

    os << ind.next() << "target: \n";
    target->dump(os, interner, ind.next(2));
    os << ind.next() << "cases: [\n";

    for (const auto& c : cases) {
        os << ind.next(2) << "{\n";
        os << ind.next(3) << "values: [\n";
        for (const auto* val : c.values) { val->dump(os, interner, ind.next(3)); }
        os << ind.next(3) << "]\n";
        dumpBlock(os, "body", interner, ind.next(3), c.body);
        os << ind.next(2) << "}\n";
    }

    if (!defaultBody.empty()) { dumpBlock(os, "default body", interner, ind.next(), defaultBody); }

    os << ind << "}\n";
}

void VariableDeclarationStatement::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "VariableDeclarationStatement", *this);
    os << ind.next() << "name: " << interner.get_view(name) << "\n";
    os << ind.next() << "isMutable: " << (isMutable ? "true" : "false") << "\n";
    os << ind.next() << "visibility: " << (visibility == Visibility::Public ? "Public" : "Private") << "\n";

    os << ind.next() << "value: \n";
    if (value != nullptr) {
        value->dump(os, interner, ind.next(2));
    } else {
        os << ind.next(2) << "null\n";
    }

    os << ind.next() << "type: \n";
    if (type == nullptr) {
        os << ind.next(2) << "auto\n";
    } else if (type->semanticType != nullptr) {
        os << ind.next() << type->semanticType->toString();
    } else {
        type->dump(os, interner, ind.next(2));
    }

    os << ind << "}\n";
}

void WhileLoopStatement::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "WhileLoopStatement", *this);
    os << ind.next() << "Is Do-While: " << (isDoWhile ? "true" : "false") << "\n";
    os << ind.next() << "condition: \n";
    condition->dump(os, interner, ind.next(2));
    dumpBlock(os, "body", interner, ind.next(), body);
    os << ind << "}\n";
}

// Expressions

void AggregateInstantiationExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "AggregateInstantiationExpression", *this);
    os << ind.next() << "base:\n";
    base->dump(os, interner, ind.next(2));

    os << ind.next() << "fields: [\n";
    for (const AggregateInstantiationField& field : fields) {
        os << ind.next(2) << "{\n";
        os << ind.next(3) << "name: " << interner.get_view(field.name) << "\n";
        os << ind.next(3) << "value: \n";
        field.value->dump(os, interner, ind.next(4));
        os << ind.next(2) << "}\n";
    }
    os << ind.next() << "]\n";
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void AggregateLiteralExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "AggregateLiteralExpression", *this);
    os << ind.next() << "Elements {\n";
    for (const Expression* element : elements) { element->dump(os, interner, ind.next(2)); }
    os << ind.next() << "}\n";
}

void AlignofExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "AlignofExpression", *this);
    os << ind.next() << "type: \n";
    type->dump(os, interner, ind.next(2));
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void ArrayLiteralExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ArrayLiteralExpression", *this);

    os << ind.next() << "elements: [\n";

    for (const Expression* element : elements) {
        os << ind.next(2) << "{\n";
        element->dump(os, interner, ind.next(3));
        os << ind.next(2) << "}\n";
    }

    os << ind.next() << "]\n";
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void AssignmentExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "AssignmentExpression", *this);
    os << ind.next() << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << ind.next() << "assignee: \n";
    assignee->dump(os, interner, ind.next(2));
    os << ind.next() << "value: \n";
    value->dump(os, interner, ind.next(2));
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void BinaryExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "BinaryExpression", *this);
    os << ind.next() << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << ind.next() << "left: \n";
    left->dump(os, interner, ind.next(2));
    os << ind.next() << "right: \n";
    right->dump(os, interner, ind.next(2));
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void BoolLiteralExpression::dump(std::ostream& os, const utils::StringInterner&  interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "BoolLiteralExpression", *this);
    os << ind.next() << "value: " << toString(interner);
    os << "\n";
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void CharLiteralExpression::dump(std::ostream& os, const utils::StringInterner&  /*unused*/, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "CharLiteralExpression", *this);
    os << ind.next() << "value: '" << lexer::codepointToUTF8(value) << "'\n";
    os << ind.next() << "code point: " << static_cast<std::int32_t>(value) << "\n";
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void FunctionCallExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "FunctionCallExpression", *this);
    os << ind.next() << "callee: \n";
    callee->dump(os, interner, ind.next(2));
    os << ind.next() << "arguments: [\n";

    for (const Expression* arg : arguments) {
        os << ind.next(2) << "{\n";
        arg->dump(os, interner, ind.next(3));
        os << ind.next(2) << "}\n";
    }

    os << ind.next() << "]\n";
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void GenericInstantiationExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "GenericInstantiationExpression", *this);
    os << ind.next() << "identifier: \n";
    identifier->dump(os, interner, ind.next(2));
    os << ind.next() << "generic types: [\n";

    for (const Type* type : types) { 
        os << ind.next(2);
        type->dump(os, interner, ind.next(3)); 
    }

    os << ind.next() << "]\n";
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void IdentifierExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "IdentifierExpression", *this);
    os << ind.next() << "name: " << interner.get_view(name) << "\n";
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void IndexExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "IndexExpression", *this);
    os << ind.next() << "variable: \n";
    variable->dump(os, interner, ind.next(2));
    os << ind.next() << "index: \n";
    index->dump(os, interner, ind.next(2));
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void MemberAccessExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "MemberAccessExpression", *this);
    os << ind.next() << "object: \n";
    object->dump(os, interner, ind.next(2));
    os << ind.next() << "property: " << interner.get_view(field) << "\n";
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void NumberLiteralExpression::dump(std::ostream& os, const utils::StringInterner&  interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "NumberLiteralExpression", *this);
    os << ind.next() << "value: " << toString(interner) << "\n";
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void PostfixExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "PostfixExpression", *this);
    os << ind.next() << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << ind.next() << "operand: \n";
    left->dump(os, interner, ind.next(2));
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void PrefixExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "PrefixExpression", *this);
    os << ind.next() << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << ind.next() << "operand: \n";
    right->dump(os, interner, ind.next(2));
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void ScopeResolutionExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ScopeResolutionExpression", *this);
    os << ind.next() << "scope: \n";
    scope->dump(os, interner, ind.next(2));
    os << ind.next() << "element: \n";
    element->dump(os, interner, ind.next(2));
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void SizeofExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "SizeofExpression", *this);
    os << ind.next() << "type: \n";
    type->dump(os, interner, ind.next(2));
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void StringLiteralExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "StringLiteralExpression", *this);
    os << ind.next() << "value: " << toString(interner) << "\n";
    os << ind.next() << "byte length: " << interner.get_view(value).length() << "\n";
    os << ind.next() << "character length: " << utf8Length(interner.get_view(value)) << "\n";
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void TernaryExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "TernaryExpression", *this);
    os << ind.next() << "condition: \n";
    condition->dump(os, interner, ind.next(2));
    os << ind.next() << "if true: \n";
    ifTrue->dump(os, interner, ind.next(2));
    os << ind.next() << "if false: \n";
    ifFalse->dump(os, interner, ind.next(2));
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void TypeCastExpression::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "TypeCastExpression", *this);
    os << ind.next() << "expression: \n";
    originalValue->dump(os, interner, ind.next(2));
    os << ind.next() << "target type: \n";
    targetType->dump(os, interner, ind.next(2));
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void UninitializedExpression::dump(std::ostream& os, const utils::StringInterner&  /*unused*/, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "Uninit", *this);
    os << ind << "}\n";
}

// Types

void AggregateType::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "AggregateType", *this);
    os << ind.next() << "fields: [\n";

    for (const auto* field : fieldTypes) {
        os << ind.next(2) << "{\n";
        field->dump(os, interner, ind.next(3));
        os << ind.next(2) << "}\n";
    }
    dumpSemanticType(os, ind.next(), semanticType);

    os << ind.next() << "]\n";
    os << ind << "}\n";
}

void ArrayType::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ArrayType", *this);
    os << ind.next() << "elementType:\n";
    elementType->dump(os, interner, ind.next(2));

    if (lengthExpression != nullptr) {
        os << ind.next() << "length:\n";
        lengthExpression->dump(os, interner, ind.next(2));
    }
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void FunctionType::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "FunctionType", *this);
    os << ind.next() << "parameter types: [\n";
    for (const auto& paramType : parameterTypes) {
        os << ind.next(2) << (paramType.isMutable ? "mut " : "") << "\n";
        paramType.type->dump(os, interner, ind.next(3));
    }
    os << ind.next() << "]\n";

    os << ind.next() << "return type: ";
    if (returnType != nullptr) {
        os << "\n";
        returnType->dump(os, interner, ind.next(2));
    } else {
        os << "void\n";
    }
    dumpSemanticType(os, ind.next(), semanticType);

    os << ind << "}\n";
}

void IdentifierType::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "IdentifierType", *this);
    os << ind.next() << "name: " << interner.get_view(name) << "\n";
    os << ind.next() << "primitive type: " << primitiveTypeToString(primitiveType) << "\n";
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void GenericInstantiationType::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "GenericInstantiationType", *this);

    os << ind.next() << "base: ";
    baseType->dump(os, interner, ind.next());
    os << "\n";
    os << ind.next() << "generic types: [\n";

    for (const auto* type : typeParameters) { type->dump(os, interner, ind.next(2)); }
    dumpSemanticType(os, ind.next(), semanticType);

    os << ind.next() << "]\n";
    os << ind << "}\n";
}

void PointerType::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "PointerType", *this);
    os << ind.next() << "base type: \n";
    baseType->dump(os, interner, ind.next(2));
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

void ScopedType::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "ScopedType", *this);
    os << ind.next() << "qualifier: \n";
    scope->dump(os, interner, ind.next(2));
    os << ind.next() << "base type: \n";
    type->dump(os, interner, ind.next(2));
    os << ind << "}\n";
}

void TypeofType::dump(std::ostream& os, const utils::StringInterner& interner, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "TypeofType", *this);
    os << ind.next() << "expression: ";
    expression->dump(os, interner, ind.next());
    dumpSemanticType(os, ind.next(), semanticType);
    os << ind << "}\n";
}

// Errors

void PoisonedStatement::dump(std::ostream& os, const utils::StringInterner&  /*unused*/, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "PoisonedStatement", *this);
    os << ind << "}\n";
}
void PoisonedExpression::dump(std::ostream& os, const utils::StringInterner&  /*unused*/, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "PoisonedExpression", *this);
    os << ind << "}\n";
}
void PoisonedType::dump(std::ostream& os, const utils::StringInterner&  /*unused*/, std::size_t indent) const {
    const Indent ind{indent};
    dumpHeader(os, ind, "PoisonedType", *this);
    os << ind << "}\n";
}

}  // namespace Manganese::ast

#endif  // MN_DEBUG