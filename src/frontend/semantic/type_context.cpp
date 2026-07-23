#include <stddef.h>

#include <core.hpp>
#include <frontend/ast/ast_base.hpp>
#include <frontend/semantic.hpp>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace Manganese {
namespace semantic {

std::string Aggregate::toString() const {
    std::string result = name.empty() ? "aggregate{" : (std::string(name) + " { ");
    for (size_t i = 0; i < fields.size(); ++i) {
        if (!fields[i].name.empty()) { result += std::string(fields[i].name) + ": "; }
        result += fields[i].type->toString();
        if (i != fields.size() - 1) [[likely]] { result += ", "; }
    }
    result += "}";
    return result;
}

std::string Array::toString() const { return std::format("{}[{}]", elementType->toString(), length); }

std::string Function::toString() const {
    std::string result = "func(";
    for (size_t i = 0; i < parameterTypes.size(); ++i) {
        const auto& param = parameterTypes[i];
        if (param.isMutable) { result += "mut "; }
        result += param.type->toString();
        if (i != parameterTypes.size() - 1) [[likely]] { result += ", "; }
    }
    result += ")";
    return result;
}

std::string GenericInstance::toString() const {
    std::string result = baseType->toString() + "@[";
    for (std::size_t i = 0; i < typeArguments.size(); ++i) {
        result += typeArguments[i]->toString();
        if (i != typeArguments.size() - 1) [[likely]] { result += ", "; }
    }
    result += ']';
    return result;
}

std::string Pointer::toString() const {
    return std::format("{}ptr {}", (isMutable ? "mut " : ""), baseType->toString());
}

constexpr inline size_t GOLDEN_RATIO = (sizeof(size_t) == 8) ? 0x9E3779B97F4A7C15ULL  // 64-bit fraction
                                                             : 0x9E3779B9U;  // 32-bit fraction

inline size_t hash_combine(size_t seed, size_t value) noexcept {
    return seed ^= value + GOLDEN_RATIO + (seed << 6) + (seed >> 2);
}

size_t TypeLookup::operator()(const SemanticType* t) const noexcept {
    if (!t) { return 0; }
    // start by hashing the type kind (isolates primitives, pointers, etc)
    size_t hash = std::hash<kind_int_t>{}(static_cast<kind_int_t>(t->kind));
    switch (t->kind) {
        case Kind::Aggregate: {
            auto* aggregate = static_cast<const Aggregate*>(t);
            // For an anonymous aggregate, hash the fields
            // Use the Boost Hash Combine algorithm
            if (aggregate->name.empty()) {
                for (const auto& field : aggregate->fields) {
                    // the bitwise shifts scramble the bits of previous fields
                    // since order matters (e.g. aggregate{int, bool} should hash differently to aggregate{bool, int})
                    hash = hash_combine(hash, std::hash<std::string_view>{}(field.name));
                    hash = hash_combine(hash, std::hash<const SemanticType*>{}(field.type));
                }
            } else {
                // Since named aggregates must be unique we can just hash their names
                hash = hash_combine(hash, std::hash<std::string_view>{}(aggregate->name));
            }
            return hash;
        }
        case Kind::Array: {
            // Since array types have a fixed structure we can just hash the member types
            auto* array = static_cast<const Array*>(t);
            hash = hash_combine(hash, std::hash<const SemanticType*>{}(array->elementType));
            return hash_combine(hash, std::hash<size_t>{}(array->length));
        }
        case Kind::Function: {
            auto* function = static_cast<const Function*>(t);
            // Mix the return type first to establish the base function signature
            hash = hash_combine(hash, std::hash<const SemanticType*>{}(function->returnType));
            // Hash in each parameter sequentially
            // Include the type and the mutability flag (e.g., func(int) and func(mut int) are different signatures)
            for (const auto& param : function->parameterTypes) {
                hash = hash_combine(hash, std::hash<const SemanticType*>{}(param.type));
                hash = hash_combine(hash, std::hash<bool>{}(param.isMutable));
            }
            return hash;
        }
        case Kind::Generic: {
            auto* generic = static_cast<const GenericInstance*>(t);
            // Mix the base generic template type (e.g., the List in List@[int])
            hash = hash_combine(hash, std::hash<const SemanticType*>{}(generic->baseType));
            for (const auto* arg : generic->typeArguments) {
                hash = hash_combine(hash, std::hash<const SemanticType*>{}(arg));
            }
            return hash;
        }
        case Kind::Pointer: {
            // like arrays, just hash the fields
            auto* pointer = static_cast<const Pointer*>(t);
            hash = hash_combine(hash, std::hash<const SemanticType*>{}(pointer->baseType));
            return hash_combine(hash, std::hash<bool>{}(pointer->isMutable));
        }
        case Kind::Primitive:
            // Since each primitive value is unique we can just hash the enum type
            return hash_combine(hash, std::hash<prim_int_t>{}(static_cast<prim_int_t>(t->primitiveType)));
        default: ASSERT_UNREACHABLE("Unknown semantic type kind in TypeLookup hash");
    }
}

bool TypeLookup::operator()(const SemanticType* lhs, const SemanticType* rhs) const noexcept {
    if (lhs == rhs) { return true; }
    if (!lhs || !rhs) { return false; }  // no deduced type; can't be equal
    if (lhs->kind != rhs->kind) { return false; }

    switch (lhs->kind) {
        case Kind::Primitive: return lhs->primitiveType == rhs->primitiveType;
        case Kind::Pointer: {
            auto* left = static_cast<const Pointer*>(lhs);
            auto* right = static_cast<const Pointer*>(rhs);
            return (left->baseType == right->baseType) && (left->isMutable == right->isMutable);
        }
        case Kind::Array: {
            auto* left = static_cast<const Array*>(lhs);
            auto* right = static_cast<const Array*>(rhs);
            return (left->elementType == right->elementType) && (left->length == right->length);
        }
        case Kind::Aggregate: {
            auto* left = static_cast<const Aggregate*>(lhs);
            auto* right = static_cast<const Aggregate*>(rhs);

            return (left->name == right->name) && (left->fields == right->fields);
        }
        case Kind::Function: {
            auto* left = static_cast<const Function*>(lhs);
            auto* right = static_cast<const Function*>(rhs);
            return (left->returnType == right->returnType) && (left->parameterTypes == right->parameterTypes);
        }
        case Kind::Generic: {
            auto* left = static_cast<const GenericInstance*>(lhs);
            auto* right = static_cast<const GenericInstance*>(rhs);
            return (left->baseType == right->baseType) && (left->typeArguments == right->typeArguments);
        }
        default: ASSERT_UNREACHABLE("Unknown semantic type kind in TypeLookup search");
    }
}

const SemanticType* TypeContext::getPrimitive(ast::PrimitiveType_t primitive) const noexcept {
    return &_primitives[static_cast<unsigned>(primitive)];
}

const SemanticType* TypeContext::getPointer(const SemanticType* baseType, bool isMutable) {
    Pointer tmp(baseType, isMutable);
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<Pointer>(baseType, isMutable);
    _cache.insert(heapAlloc);
    return heapAlloc;
}

const SemanticType* TypeContext::getArray(const SemanticType* elementType, size_t length) {
    Array tmp(elementType, length);
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<Array>(elementType, length);
    _cache.insert(heapAlloc);
    return heapAlloc;
}

const SemanticType* TypeContext::getAnonymousAggregate(std::vector<const SemanticType*>&& fieldTypes) {
    Aggregate tmp(std::move(fieldTypes));
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<Aggregate>(std::move(tmp.fields));
    _cache.insert(heapAlloc);
    return heapAlloc;
}

const SemanticType* TypeContext::getNamedAggregate(std::string_view name, std::vector<AggregateField>&& fieldTypes) {
    // Named types are nominal: they are unique by their declaration name.
    Aggregate tmp(std::move(fieldTypes), name);
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<Aggregate>(std::move(tmp.fields), name);
    _cache.insert(heapAlloc);
    return heapAlloc;
}

const SemanticType* TypeContext::getFunction(std::vector<Parameter>&& parameterTypes, const SemanticType* returnType) {
    Function tmp(std::move(parameterTypes), returnType);
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<Function>(std::move(tmp.parameterTypes), returnType);
    _cache.insert(heapAlloc);
    return heapAlloc;
}

const SemanticType* TypeContext::getGenericInstance(const SemanticType* baseType,
                                                    std::vector<const SemanticType*>&& typeArguments) {
    GenericInstance tmp(baseType, std::move(typeArguments));
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<GenericInstance>(baseType, std::move(tmp.typeArguments));
    _cache.insert(heapAlloc);
    return heapAlloc;
}
}  // namespace semantic
}  // namespace Manganese
