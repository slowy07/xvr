#ifndef XVR_TYPE_HPP
#define XVR_TYPE_HPP

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../xvr_literal.h"

namespace xvr {

enum class TypeKind {
    Null,
    Void,
    Boolean,
    Integer,
    Float,
    String,
    Array,
    Dictionary,
    Function,
    Identifier,
    Type,
    Opaque,
    Any,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float16,
    Float32,
    Float64
};

class Type {
public:
    TypeKind kind;
    std::vector<std::shared_ptr<Type>> subtypes;
    bool isConstant{false};
    int bitWidth{0};

    Type() : kind(TypeKind::Null) {}
    explicit Type(TypeKind k) : kind(k) {}

    bool isPrimitive() const;
    bool isNumeric() const;
    bool isInteger() const;
    bool isFloat() const;
    bool isPointer() const;
    bool isArray() const;
    bool isDictionary() const;
    bool isFunction() const;

    static std::shared_ptr<Type> fromLiteralType(Xvr_LiteralType literalType);
    Xvr_LiteralType toLiteralType() const;

    std::string toString() const;
    bool operator==(const Type& other) const;
    bool operator!=(const Type& other) const { return !(*this == other); }
};

using TypePtr = std::shared_ptr<Type>;
using TypeMap = std::unordered_map<std::string, TypePtr>;

class TypeEnvironment {
public:
    void define(const std::string& name, TypePtr type);
    std::optional<TypePtr> lookup(const std::string& name) const;
    void enterScope();
    void exitScope();

private:
    std::vector<TypeMap> scopes_;
};

}  // namespace xvr

#endif  // XVR_TYPE_HPP