#include <stddef.h>

#include <core.hpp>
#include <frontend/ast/ast_base.hpp>
#include <frontend/semantic.hpp>
#include <functional>

namespace Manganese {
namespace semantic {

std::string Aggregate::toString() const {
    std::string result = name.empty() ? "aggregate" : std::string(name);
    result += '{';
    for (size_t i = 0; i < fieldTypes.size(); ++i) {
        result += fieldTypes[i]->toString();
        if (i != fieldTypes.size() - 1) [[likely]] { result += ", "; }
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

size_t TypeLookup::operator()(const SemanticType* t) const noexcept {
    // start by hashing the type kind (isolates primitives, pointers, etc)
    size_t hash = std::hash<kind_int_t>{}(static_cast<kind_int_t>(t->kind));
    switch (t->kind) {
        case Kind::Aggregate: {
            auto* aggregate = static_cast<const Aggregate*>(t);
            // For an anonymous aggregate, hash the fields
            // Use the Boost Hash Combine algorithm
            if (aggregate->name.empty()) {
                for (const auto* fieldType : aggregate->fieldTypes) {
                    // the bitwise shifts scramble the bits of previous fields
                    // since order matteres (e.g. aggregate{int, bool} should hash differently to aggregate{bool, int})
                    hash ^= std::hash<const SemanticType*>{}(fieldType) + GOLDEN_RATIO + (hash << 6) + (hash >> 2);
                }
            } else {
                // Since named aggregates must be unique we can just hash their names
                hash ^= std::hash<std::string_view>{}(aggregate->name) << 1;
            }
            return hash;
        }
        case Kind::Array: {
            // Since array types have a fixed structure we can just hash the member types
            auto* array = static_cast<const Array*>(t);
            return hash ^ (std::hash<const SemanticType*>{}(array->elementType) << 1)
                ^ (std::hash<size_t>{}(array->length) << 2);
        }
        case Kind::Function: {
            auto* function = static_cast<const Function*>(t);
            // Mix the return type first to establish the base function signature
            hash ^= std::hash<const SemanticType*>{}(function->returnType) << 1;
            // Hash in each parameter sequentially
            // Include the type and the mutability flag (e.g., func(int) and func(mut int) are different signatures)
            for (const auto& param : function->parameterTypes) {
                hash ^= std::hash<const SemanticType*>{}(param.type) ^ (std::hash<bool>{}(param.isMutable) << 1);
            }
            return hash;
        }
        case Kind::Generic: {
            auto* generic = static_cast<const GenericInstance*>(t);
            // Mix the base generic template type (e.g., the List in List@[int])
            hash ^= std::hash<const SemanticType*>{}(generic->baseType) << 1;
            // Mix the specific type arguments injected into this instantiation instance.
            // re-use the bit shifting trick from aggregates since generics can have variable numbers of type parameters
            for (const auto* arg : generic->typeArguments) {
                hash ^= std::hash<const SemanticType*>{}(arg) + GOLDEN_RATIO + (hash << 6) + (hash >> 2);
            }
            return hash;
        }
        case Kind::Pointer: {
            // like arrays, just hash the fields
            auto* pointer = static_cast<const Pointer*>(t);
            return hash ^ (std::hash<const SemanticType*>{}(pointer->baseType) << 1)
                ^ (std::hash<bool>{}(pointer->isMutable) << 2);
        }
        case Kind::Primitive:
            // Since each primitive value is unique we can just hash the enum type
            return hash ^ (std::hash<prim_int_t>{}(static_cast<prim_int_t>(t->primitiveType)));
        default: ASSERT_UNREACHABLE("Unknown semantic type kind in TypeLookup hash");
    }
}

bool TypeLookup::operator()(const SemanticType* lhs, const SemanticType* rhs) const noexcept {
    if (lhs->kind != rhs->kind) { return false; }

    switch (lhs->kind) {
        case Kind::Primitive: return lhs->primitiveType == rhs->primitiveType;
        case Kind::Pointer: {
            auto* left = static_cast<const Pointer*>(lhs);
            auto* right = static_cast<const Pointer*>(rhs);
            return left->baseType == right->baseType && left->isMutable == right->isMutable;
        }
        case Kind::Array: {
            auto* left = static_cast<const Array*>(lhs);
            auto* right = static_cast<const Array*>(rhs);
            return left->elementType == right->elementType && left->length == right->length;
        }
        case Kind::Aggregate: {
            auto* left = static_cast<const Aggregate*>(lhs);
            auto* right = static_cast<const Aggregate*>(rhs);

            return (left->name == right->name) && (left->fieldTypes == right->fieldTypes);
        }
        case Kind::Function: {
            auto* left = static_cast<const Function*>(lhs);
            auto* right = static_cast<const Function*>(rhs);
            return (left->returnType == right->returnType) && (left->parameterTypes == right->parameterTypes);
        }
        case Kind::Generic: {
            auto* left = static_cast<const GenericInstance*>(lhs);
            auto* right = static_cast<const GenericInstance*>(rhs);
            return left->baseType == right->baseType && left->typeArguments == right->typeArguments;
        }
        default: ASSERT_UNREACHABLE("Unknown semantic type kind in TypeLookup search");
    }
}

TypeContext::TypeContext() noexcept {
    for (int i = 0; i < static_cast<int>(ast::PrimitiveType_t::boolean); ++i) {
        _primitives[i] = SemanticType(Kind::Primitive, static_cast<ast::PrimitiveType_t>(i));
    }
}

const SemanticType* TypeContext::getPrimitive(ast::PrimitiveType_t primitive) const noexcept {
    return &_primitives[static_cast<int>(primitive)];
}

const SemanticType* TypeContext::getPointer(const SemanticType* baseType, bool isMutable) {
    Pointer tmp(baseType, isMutable);
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<Pointer>(std::move(tmp));
    _cache.insert(heapAlloc);
    return heapAlloc;
}

const SemanticType* TypeContext::getArray(const SemanticType* elementType, size_t length) {
    Array tmp(elementType, length);
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<Array>(std::move(tmp));
    _cache.insert(heapAlloc);
    return heapAlloc;
}

const SemanticType* TypeContext::getAnonymousAggregate(std::vector<const SemanticType*>&& fieldTypes) {
    Aggregate tmp(std::move(fieldTypes));
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<Aggregate>(std::move(tmp));
    _cache.insert(heapAlloc);
    return heapAlloc;
}

const SemanticType* TypeContext::getNamedAggregate(std::string_view name,
                                                   std::vector<const SemanticType*>&& fieldTypes) {
    // Named types are nominal: they are unique by their declaration name.
    Aggregate tmp(std::move(fieldTypes), name);
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<Aggregate>(std::move(tmp));
    _cache.insert(heapAlloc);
    return heapAlloc;
}

const SemanticType* TypeContext::getFunction(std::vector<Parameter>&& parameterTypes, const SemanticType* returnType) {
    Function tmp(std::move(parameterTypes), returnType);
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<Function>(std::move(tmp));
    _cache.insert(heapAlloc);
    return heapAlloc;
}

const SemanticType* TypeContext::getGenericInstance(const SemanticType* baseType,
                                                    std::vector<const SemanticType*>&& typeArguments) {
    GenericInstance tmp(baseType, std::move(typeArguments));
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<GenericInstance>(std::move(tmp));
    _cache.insert(heapAlloc);
    return heapAlloc;
}
}  // namespace semantic
}  // namespace Manganese
