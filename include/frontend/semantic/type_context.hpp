#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_TYPE_CONTEXT_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_TYPE_CONTEXT_HPP 1

#include <cstdint>
#include <frontend/ast.hpp>
#include <mnstl/chunk_allocator.hxx>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Manganese {
namespace semantic {

class TypeContext;

enum class Kind : uint8_t {
    Aggregate,
    Array,
    Function,
    Generic,
    Pointer,
    Primitive,
};

struct SemanticType {
    Kind kind;
    ast::PrimitiveType_t primitiveType;

    constexpr SemanticType(Kind kind_, ast::PrimitiveType_t primitive = ast::PrimitiveType_t::not_primitive) noexcept :
        kind(kind_), primitiveType(primitive) {}

    virtual ~SemanticType() noexcept = default;

    constexpr bool isAggregate() const noexcept { return kind == Kind::Aggregate; }
    constexpr bool isArray() const noexcept { return kind == Kind::Array; }
    constexpr bool isFunction() const noexcept { return kind == Kind::Function; }
    constexpr bool isGeneric() const noexcept { return kind == Kind::Generic; }
    constexpr bool isPointer() const noexcept { return kind == Kind::Pointer; }
    constexpr bool isPrimitive() const noexcept { return kind == Kind::Primitive; }

   private:
    constexpr SemanticType() noexcept : kind(Kind::Primitive), primitiveType(ast::PrimitiveType_t::not_primitive) {}

    friend class TypeContext;
};

struct Aggregate final : public SemanticType {
    std::vector<const SemanticType*> fieldTypes;
    std::string_view name;

    Aggregate(std::vector<const SemanticType*>&& types, std::string_view aggregateName = "") noexcept :
        SemanticType(Kind::Aggregate), fieldTypes(std::move(types)), name(aggregateName) {}

    ~Aggregate() override = default;
};

struct Array final : public SemanticType {
    const SemanticType* elementType;
    const size_t length;

    Array(const SemanticType* baseType, size_t len) noexcept :
        SemanticType(Kind::Array), elementType(baseType), length(len) {}
    ~Array() override = default;
};

struct Parameter {
    bool isMutable;
    const SemanticType* type;

    bool operator==(const Parameter&) const noexcept = default;
};

struct Function final : public SemanticType {
    const SemanticType* returnType;
    std::vector<Parameter> parameterTypes;

    Function(std::vector<Parameter> params, const SemanticType* ret) noexcept :
        SemanticType(Kind::Function, static_cast<ast::PrimitiveType_t>(0)),
        returnType(ret),
        parameterTypes(std::move(params)) {}

    ~Function() override = default;
};

struct GenericInstance final : public SemanticType {
    const SemanticType* baseType;
    std::vector<const SemanticType*> typeArguments;

    GenericInstance(const SemanticType* base, std::vector<const SemanticType*> args) noexcept :
        SemanticType(Kind::Generic), baseType(base), typeArguments(std::move(args)) {}

    ~GenericInstance() override = default;
};

struct Pointer final : public SemanticType {
    const SemanticType* baseType;
    const bool isMutable;

    Pointer(const SemanticType* base, bool isMut) noexcept :
        SemanticType(Kind::Pointer), baseType(base), isMutable(isMut) {}
    ~Pointer() override = default;
};

struct TypeLookup {
    using is_transparent = void;  // enables heterogenous lookup inside std::unordered_set
    using kind_int_t = std::underlying_type_t<Kind>;
    using prim_int_t = std::underlying_type_t<ast::PrimitiveType_t>;

    size_t operator()(const SemanticType* t) const noexcept;

    bool operator()(const SemanticType* lhs, const SemanticType* rhs) const noexcept;
};

class TypeContext {
   private:
    mnstl::chunk_allocator _allocator;

    std::unordered_set<const SemanticType*, TypeLookup, TypeLookup> _cache;
    SemanticType _primitives[static_cast<int>(ast::PrimitiveType_t::boolean) + 1];

   public:
    TypeContext() noexcept;
    ~TypeContext() = default;

    TypeContext(const TypeContext&) = delete;
    TypeContext& operator=(const TypeContext&) = delete;

    const SemanticType* getPrimitive(ast::PrimitiveType_t primitive) const noexcept;

    const SemanticType* getPointer(const SemanticType* baseType, bool isMutable);

    const SemanticType* getArray(const SemanticType* baseType, size_t length);

    const SemanticType* getAnonymousAggregate(std::vector<const SemanticType*>&& fieldTypes);

    const SemanticType* getNamedAggregate(std::string_view name, std::vector<const SemanticType*>&& fieldTypes);

    const SemanticType* getFunction(std::vector<Parameter>&& parameterTypes, const SemanticType* returnType);

    const SemanticType* getGenericInstance(const SemanticType* baseType, std::vector<const SemanticType*>&& typeArguments);
};

}  // namespace semantic
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_TYPE_CONTEXT_HPP
