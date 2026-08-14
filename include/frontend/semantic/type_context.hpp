#ifndef MANGANESE_INCLUDE_FRONTEND_SEMANTIC_TYPE_CONTEXT_HPP
#define MANGANESE_INCLUDE_FRONTEND_SEMANTIC_TYPE_CONTEXT_HPP 1

#include <array>
#include <cstddef>
#include <cstdint>
#include <frontend/ast.hpp>
#include <mnstl/chunk_allocator.hxx>
#include <mnstl/enum_matches.hxx>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Manganese::semantic {

struct SemanticType;
class TypeContext;

using TypeList = std::vector<const SemanticType*>;

enum class Kind : std::uint8_t {
    Aggregate,
    Array,
    Enum,
    Function,
    Generic,
    Pointer,
    Primitive,
    Void,
};

enum class ResolutionStatus : std::int8_t {
    Failure = -1,
    InProgress = 0,
    Success = 1,
    NotStarted = 2,
};

struct SemanticType {
    const Kind kind;
    const ast::PrimitiveType_t primitiveType;

    constexpr explicit SemanticType(Kind _kind,
                                    ast::PrimitiveType_t primitive = ast::PrimitiveType_t::not_primitive) noexcept :
        kind(_kind), primitiveType(primitive) {}

    virtual ~SemanticType() noexcept = default;

    constexpr bool isAggregate() const noexcept { return kind == Kind::Aggregate; }
    constexpr bool isArray() const noexcept { return kind == Kind::Array; }
    constexpr bool isEnum() const noexcept { return kind == Kind::Enum; }
    constexpr bool isFunction() const noexcept { return kind == Kind::Function; }
    constexpr bool isGeneric() const noexcept { return kind == Kind::Generic; }
    constexpr bool isPointer() const noexcept { return kind == Kind::Pointer; }
    constexpr bool isPrimitive() const noexcept { return kind == Kind::Primitive; }
    constexpr bool isVoid() const noexcept { return kind == Kind::Primitive; }

    constexpr bool isBoolean() const noexcept {
        return isPrimitive() && primitiveType == ast::PrimitiveType_t::boolean;
    }

    constexpr bool isUnsignedInteger() const noexcept {
        using enum ast::PrimitiveType_t;
        return isPrimitive() && mnstl::enum_matches(primitiveType, u8, u16, u32, u64, u128);
    }
    constexpr bool isSignedInteger() const noexcept {
        using enum ast::PrimitiveType_t;
        return isPrimitive() && mnstl::enum_matches(primitiveType, i8, i16, i32, i64, i128);
    }
    constexpr bool isInteger() const noexcept { return isSignedInteger() || isUnsignedInteger(); }
    constexpr bool isFloat() const noexcept {
        using enum ast::PrimitiveType_t;
        return isPrimitive() && mnstl::enum_matches(primitiveType, f32, f64);
    }

    constexpr bool isNumeric() const noexcept { return isInteger() || isFloat(); }

    virtual std::string toString() const { return std::string(ast::primitiveTypeToString(primitiveType)); }

   private:
    constexpr SemanticType() noexcept : kind(Kind::Primitive), primitiveType(ast::PrimitiveType_t::not_primitive) {}

    friend class TypeContext;
};

struct AggregateField {
    std::string_view name;  // empty for anonymous aggregates
    const SemanticType* type;

    friend bool operator==(const AggregateField&, const AggregateField&) noexcept = default;
};

struct Aggregate final : public SemanticType {
    mutable std::vector<AggregateField> fields;
    const std::string_view name;
    mutable ResolutionStatus status;

    Aggregate(std::vector<AggregateField>&& fieldTypes, std::string_view aggregateName = "") noexcept :
        SemanticType(Kind::Aggregate),
        fields(std::move(fieldTypes)),
        name(aggregateName),
        status(ResolutionStatus::NotStarted) {}

    // For anonymous aggregates
    Aggregate(TypeList&& rawTypes) noexcept : SemanticType(Kind::Aggregate), name(), status(ResolutionStatus::Success) {
        fields.reserve(rawTypes.size());
        for (const SemanticType* t : rawTypes) { fields.push_back(AggregateField{.name = "", .type = t}); }
    }

    const SemanticType* getFieldType(const std::string_view& fieldName) const noexcept {
        for (const AggregateField& field : fields) {
            if (field.name == fieldName) { return field.type; }
        }
        return nullptr;
    }

    const SemanticType* getFieldType(std::size_t index) const noexcept {
        if (index < fields.size()) [[likely]] { return fields[index].type; }
        return nullptr;
    }

    ~Aggregate() override = default;

    std::string toString() const override;
};

struct Array final : public SemanticType {
    const SemanticType* elementType;
    const std::size_t length;

    Array(const SemanticType* baseType, std::size_t len) noexcept :
        SemanticType(Kind::Array), elementType(baseType), length(len) {}
    ~Array() override = default;
    std::string toString() const override;
};

struct Variant {
    std::string_view name;
    std::optional<std::int64_t> value = std::nullopt;
};

struct Enum final : public SemanticType {
    const std::string_view name;
    mutable const SemanticType* underlyingType = nullptr;
    mutable std::vector<Variant> variants;
    mutable ResolutionStatus status = ResolutionStatus::NotStarted;

    explicit Enum(std::string_view enumName, const SemanticType* defaultUnderlying = nullptr) noexcept :
        SemanticType(Kind::Enum), name(enumName), underlyingType(defaultUnderlying) {}

    bool hasVariant(std::string_view variantName) const noexcept {
        for (const auto& v : variants) {
            if (v.name == variantName) { return true; }
        }
        return false;
    }

    ~Enum() override = default;
    std::string toString() const override;
};

struct Parameter {
    bool isMutable;
    bool isVariadic;
    const SemanticType* type;

    friend bool operator==(const Parameter&, const Parameter&) noexcept = default;

    inline std::string toString() const {
        std::string result = type->toString();
        if (isVariadic) { result += "..."; }
        if (isMutable) { result = "mut " + result; }
        return result;
    }
};

struct Function final : public SemanticType {
    const SemanticType* returnType;
    std::vector<Parameter> parameterTypes;

    Function(std::vector<Parameter>&& params, const SemanticType* ret) noexcept :
        SemanticType(Kind::Function, static_cast<ast::PrimitiveType_t>(0)),
        returnType(ret),
        parameterTypes(std::move(params)) {}

    ~Function() override = default;
    std::string toString() const override;
};

struct GenericInstance final : public SemanticType {
    const SemanticType* baseType;
    TypeList typeArguments;

    GenericInstance(const SemanticType* base, TypeList&& args) noexcept :
        SemanticType(Kind::Generic), baseType(base), typeArguments(std::move(args)) {}

    ~GenericInstance() override = default;
    std::string toString() const override;
};

struct Pointer final : public SemanticType {
    const SemanticType* baseType;
    const bool isMutable;

    Pointer(const SemanticType* base, bool isMut) noexcept :
        SemanticType(Kind::Pointer), baseType(base), isMutable(isMut) {}
    ~Pointer() override = default;
    std::string toString() const override;
};

struct PrimitiveInfo {
    enum class Category {
        Int,
        UInt,
        Float,
        Char,
        Bool,
        String
    };
    Category category;
    int bitWidth = 0;
};

PrimitiveInfo getPrimitiveInfo(ast::PrimitiveType_t type);

struct Void final : public SemanticType {
    Void() noexcept : SemanticType(Kind::Void) {};
    ~Void() override = default;
    std::string toString() const override;
};

struct TypeLookup {
    using is_transparent = void;  // enables heterogenous lookup inside std::unordered_set
    using kind_int_t = std::underlying_type_t<Kind>;
    using prim_int_t = std::underlying_type_t<ast::PrimitiveType_t>;

    // Hashing
    std::size_t operator()(const SemanticType* t) const noexcept;

    // Lookup
    bool operator()(const SemanticType* lhs, const SemanticType* rhs) const noexcept;
};

constexpr inline std::size_t GOLDEN_RATIO = (sizeof(std::size_t) == 8) ? 0x9E3779B97F4A7C15ULL  // 64-bit fraction
                                                                       : 0x9E3779B9U;  // 32-bit fraction

inline std::size_t hash_combine(std::size_t seed, std::size_t value) noexcept {
    return seed ^= value + GOLDEN_RATIO + (seed << 6) + (seed >> 2);
}

class TypeContext {
   private:
    mnstl::chunk_allocator& _allocator;
    constexpr static inline unsigned NUM_PRIMITIVES = static_cast<unsigned>(ast::PrimitiveType_t::boolean) + 1;

    std::unordered_set<const SemanticType*, TypeLookup, TypeLookup> _cache;
    std::array<SemanticType, NUM_PRIMITIVES> _primitives;
    Void _voidInstance;

    template <std::size_t... Is>
    constexpr static std::array<SemanticType, sizeof...(Is)> _makePrimitives(std::index_sequence<Is...>) noexcept {
        return {SemanticType(Kind::Primitive, static_cast<ast::PrimitiveType_t>(Is))...};
    }

   public:
    explicit TypeContext(mnstl::chunk_allocator& allocator) noexcept :
        _allocator(allocator), _primitives(_makePrimitives(std::make_index_sequence<NUM_PRIMITIVES>{})) {}
    ~TypeContext() = default;

    TypeContext(const TypeContext&) = delete;
    TypeContext& operator=(const TypeContext&) = delete;

    const SemanticType* getArray(const SemanticType* elementType, std::size_t length);

    const SemanticType* getAnonymousAggregate(TypeList&& fieldTypes);

    const SemanticType* getNamedAggregate(std::string_view name, std::vector<AggregateField>&& fieldTypes);

    const SemanticType* getEnum(std::string_view name);

    const SemanticType* getFunction(std::vector<Parameter>&& parameterTypes, const SemanticType* returnType);

    const SemanticType* getGenericInstance(const SemanticType* baseType, TypeList&& typeArguments);

    const SemanticType* getPointer(const SemanticType* baseType, bool isMutable);

    const SemanticType* getPrimitive(ast::PrimitiveType_t primitive) const noexcept;

    const SemanticType* getVoid() const noexcept;

    const SemanticType* getUSizeType() const noexcept;
    const SemanticType* getSSizeType() const noexcept;
};

}  // namespace Manganese::semantic

#endif  // MANGANESE_INCLUDE_FRONTEND_SEMANTIC_TYPE_CONTEXT_HPP
