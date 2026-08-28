#include <core.hpp>
#include <cstddef>
#include <frontend/ast/ast_base.hpp>
#include <frontend/semantic.hpp>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Manganese::semantic {

PrimitiveInfo getPrimitiveInfo(ast::PrimitiveType_t type) {
    using enum ast::PrimitiveType_t;
    using Cat = PrimitiveInfo::Category;

    switch (type) {
        case i8: return {.category = Cat::Int, .bitWidth = 8};
        case i16: return {.category = Cat::Int, .bitWidth = 16};
        case i32: return {.category = Cat::Int, .bitWidth = 32};
        case i64: return {.category = Cat::Int, .bitWidth = 64};
        case i128: return {.category = Cat::Int, .bitWidth = 128};

        case u8: return {.category = Cat::UInt, .bitWidth = 8};
        case u16: return {.category = Cat::UInt, .bitWidth = 16};
        case u32: return {.category = Cat::UInt, .bitWidth = 32};
        case u64: return {.category = Cat::UInt, .bitWidth = 64};
        case u128: return {.category = Cat::UInt, .bitWidth = 128};

        case f32: return {.category = Cat::Float, .bitWidth = 32};
        case f64: return {.category = Cat::Float, .bitWidth = 64};

        case character: return {.category = Cat::Char, .bitWidth = 8};
        case boolean: return {.category = Cat::Bool, .bitWidth = 1};
        case str: return {.category = Cat::String, .bitWidth = 0};
        default: break;
    }
    return {Cat::Int, 0};
}

std::string Aggregate::toString() const {
    std::string result = name.empty() ? "aggregate{" : (std::string(name) + " { ");
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (!fields[i].name.empty()) { result += std::string(fields[i].name) + ": "; }
        result += fields[i].type->toString();
        if (i != fields.size() - 1) [[likely]] { result += ", "; }
    }
    result += "}";
    return result;
}

std::string Array::toString() const { return std::format("{}[{}]", elementType->toString(), length); }

std::string Enum::toString() const { return std::string(name); }

std::string Function::toString() const {
    std::string result = "func(";
    for (std::size_t i = 0; i < parameterTypes.size(); ++i) {
        const Parameter& param = parameterTypes[i];
        if (param.isMutable) { result += "mut "; }
        result += param.type->toString();
        if (i != parameterTypes.size() - 1) [[likely]] { result += ", "; }
    }
    result += ")";
    return result;
}

std::string GenericInstantiation::toString() const {
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

std::string Void::toString() const { return "void"; }

std::size_t TypeLookup::operator()(const SemanticType* t) const noexcept {
    if (!t) { return 0; }
    // start by hashing the type kind (isolates primitives, pointers, etc)
    std::size_t hash = std::hash<kind_int_t>{}(static_cast<kind_int_t>(t->kind));
    switch (t->kind) {
        case Kind::Aggregate: {
            const auto* aggregate = static_cast<const Aggregate*>(t);
            // For an anonymous aggregate, hash the fields
            // Use the Boost Hash Combine algorithm
            if (aggregate->name.empty()) {
                for (const AggregateField& field : aggregate->fields) {
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
            const auto* array = static_cast<const Array*>(t);
            hash = hash_combine(hash, std::hash<const SemanticType*>{}(array->elementType));
            return hash_combine(hash, std::hash<std::size_t>{}(array->length));
        }

        case Kind::Enum: {
            const auto* enumeration = static_cast<const Enum*>(t);
            return std::hash<std::string_view>{}(enumeration->name);
        }
        case Kind::Function: {
            const auto* function = static_cast<const Function*>(t);
            // Mix the return type first to establish the base function signature
            hash = hash_combine(hash, std::hash<const SemanticType*>{}(function->returnType));
            // Hash in each parameter sequentially
            // Include the type and the mutability flag (e.g., func(int) and func(mut int) are different signatures)
            for (const Parameter& param : function->parameterTypes) {
                hash = hash_combine(hash, std::hash<const SemanticType*>{}(param.type));
                hash = hash_combine(hash, std::hash<bool>{}(param.isMutable));
            }
            return hash;
        }
        case Kind::Generic: {
            const auto* generic = static_cast<const GenericInstantiation*>(t);
            // Mix the base generic template type (e.g., the List in List@[int])
            hash = hash_combine(hash, std::hash<const SemanticType*>{}(generic->baseType));
            for (const SemanticType* arg : generic->typeArguments) {
                hash = hash_combine(hash, std::hash<const SemanticType*>{}(arg));
            }
            return hash;
        }
        case Kind::Pointer: {
            // like arrays, just hash the fields
            const auto* pointer = static_cast<const Pointer*>(t);
            hash = hash_combine(hash, std::hash<const SemanticType*>{}(pointer->baseType));
            return hash_combine(hash, std::hash<bool>{}(pointer->isMutable));
        }
        case Kind::Primitive:
            // Since each primitive value is unique we can just hash the enum type
            return hash_combine(hash, std::hash<prim_int_t>{}(static_cast<prim_int_t>(t->primitiveType)));
        case Kind::Void: return hash;  // no extra logic neexex
        default: ASSERT_UNREACHABLE("Unknown semantic type kind in TypeLookup hash");
    }
}

bool TypeLookup::operator()(const SemanticType* lhs, const SemanticType* rhs) const noexcept {
    if (!lhs || !rhs) { return false; }  // no deduced type; can't be equal
    if (lhs == rhs) { return true; }
    if (lhs->kind != rhs->kind) { return false; }

    switch (lhs->kind) {
        case Kind::Primitive: return lhs->primitiveType == rhs->primitiveType;
        case Kind::Pointer: {
            const auto* left = static_cast<const Pointer*>(lhs);
            const auto* right = static_cast<const Pointer*>(rhs);
            return (left->baseType == right->baseType) && (left->isMutable == right->isMutable);
        }
        case Kind::Array: {
            const auto* left = static_cast<const Array*>(lhs);
            const auto* right = static_cast<const Array*>(rhs);
            return (left->elementType == right->elementType) && (left->length == right->length);
        }
        case Kind::Aggregate: {
            const auto* left = static_cast<const Aggregate*>(lhs);
            const auto* right = static_cast<const Aggregate*>(rhs);
            // For named aggregates, we can immediately distinguish by names
            // Only for anonymous aggregates do we need to check fields
            return (left->name == right->name) && (left->fields == right->fields);
        }
        case Kind::Enum: {
            const auto* left = static_cast<const Enum*>(lhs);
            const auto* right = static_cast<const Enum*>(rhs);
            return left->name == right->name;
        }
        case Kind::Function: {
            const auto* left = static_cast<const Function*>(lhs);
            const auto* right = static_cast<const Function*>(rhs);
            return (left->returnType == right->returnType) && (left->parameterTypes == right->parameterTypes);
        }
        case Kind::Generic: {
            const auto* left = static_cast<const GenericInstantiation*>(lhs);
            const auto* right = static_cast<const GenericInstantiation*>(rhs);
            return (left->baseType == right->baseType) && (left->typeArguments == right->typeArguments);
        }
        case Kind::Void: {
            return true;
        }
        default: ASSERT_UNREACHABLE("Unknown semantic type kind in TypeLookup search");
    }
}

const SemanticType* TypeContext::getArray(const SemanticType* elementType, std::size_t length) {
    Array tmp(elementType, length);
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<Array>(elementType, length);
    _cache.insert(heapAlloc);
    return heapAlloc;
}

const SemanticType* TypeContext::getAnonymousAggregate(TypeList&& fieldTypes) {
    Aggregate tmp(std::move(fieldTypes));
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<Aggregate>(std::move(tmp.fields));
    _cache.insert(heapAlloc);
    return heapAlloc;
}

const SemanticType* TypeContext::getNamedAggregate(std::string&& name, std::vector<AggregateField>&& fieldTypes) {
    // Named types are nominal: they are unique by their declaration name.
    Aggregate tmp(std::move(fieldTypes), std::move(name));
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<Aggregate>(std::move(tmp.fields), std::move(tmp.name));
    _cache.insert(heapAlloc);
    return heapAlloc;
}

const SemanticType* TypeContext::getEnum(std::string_view name) {
    Enum tmp(name);
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<Enum>(name);
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

const SemanticType* TypeContext::getGenericInstance(const SemanticType* baseType, TypeList&& typeArguments) {
    GenericInstantiation tmp(baseType, std::move(typeArguments));
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<GenericInstantiation>(baseType, std::move(tmp.typeArguments));
    _cache.insert(heapAlloc);
    return heapAlloc;
}

const SemanticType* TypeContext::getPointer(const SemanticType* baseType, bool isMutable) {
    Pointer tmp(baseType, isMutable);
    if (auto it = _cache.find(static_cast<const SemanticType*>(&tmp)); it != _cache.end()) { return *it; }
    auto* heapAlloc = _allocator.emplace<Pointer>(baseType, isMutable);
    _cache.insert(heapAlloc);
    return heapAlloc;
}

const SemanticType* TypeContext::getPrimitive(ast::PrimitiveType_t primitive) const noexcept {
    if (primitive == ast::PrimitiveType_t::not_primitive) {
        ASSERT_UNREACHABLE("Attempted to get a primitive type corresponding to a non-primitive value!");
    }
    return &_primitives[static_cast<unsigned>(primitive)];
}

const SemanticType* TypeContext::getVoid() const noexcept { return &_voidInstance; }

const SemanticType* TypeContext::getUSizeType() const noexcept {
#if UINTPTR_MAX == 0xFFFFFFFF
    // 32-bit Host Architecture
    return getPrimitive(ast::PrimitiveType_t::u32);
#elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
    // 64-bit Host Architecture
    return getPrimitive(ast::PrimitiveType_t::u64);
#else
#error "Unsupported pointer width / host architecture"
#endif
}

const SemanticType* TypeContext::getSSizeType() const noexcept {
#if UINTPTR_MAX == 0xFFFFFFFF
    // 32-bit Host Architecture
    return getPrimitive(ast::PrimitiveType_t::i32);
#elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
    // 64-bit Host Architecture
    return getPrimitive(ast::PrimitiveType_t::i64);
#else
#error "Unsupported pointer width / host architecture"
#endif
}

}  // namespace Manganese::semantic
