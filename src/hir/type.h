#ifndef MINI_HIR_TYPE_H_
#define MINI_HIR_TYPE_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "../span.h"
#include "printable.h"

namespace mini {

namespace hir {

class BuiltinType;
class PointerType;
class ArrayType;
class NameType;

class TypeVisitor {
public:
    virtual ~TypeVisitor() {}
    virtual void Visit(const BuiltinType &type) = 0;
    virtual void Visit(const PointerType &type) = 0;
    virtual void Visit(const ArrayType &type) = 0;
    virtual void Visit(const NameType &type) = 0;
};

class Type : public Printable {
public:
    Type(Span span) : span_(span) {}
    virtual void Accept(TypeVisitor &visitor) const = 0;
    virtual bool IsBuiltin() const { return false; }
    virtual bool IsPointer() const { return false; }
    virtual bool IsArray() const { return false; }
    virtual bool IsName() const { return false; }
    virtual BuiltinType *ToBuiltin() { return nullptr; }
    virtual PointerType *ToPointer() { return nullptr; }
    virtual ArrayType *ToArray() { return nullptr; }
    virtual NameType *ToName() { return nullptr; }
    inline Span span() const { return span_; }

private:
    Span span_;
};

class BuiltinType : public Type {
public:
    enum Kind {
        Void,
        ISize,
        Int8,
        Int16,
        Int32,
        Int64,
        USize,
        UInt8,
        UInt16,
        UInt32,
        UInt64,
        Char,
        Bool,
    };
    BuiltinType(Kind kind, Span span) : Type(span), kind_(kind) {}
    inline void Accept(TypeVisitor &visitor) const override {
        visitor.Visit(*this);
    }
    inline bool IsBuiltin() const override { return true; }
    inline BuiltinType *ToBuiltin() override { return this; }
    inline bool IsInteger() const {
        return kind_ == ISize || kind_ == Int8 || kind_ == Int16 ||
               kind_ == Int32 || kind_ == Int64 || kind_ == USize ||
               kind_ == UInt8 || kind_ == UInt16 || kind_ == UInt32 ||
               kind_ == UInt64;
    }
    void Print(PrintableContext &ctx) const override;
    inline Kind kind() const { return kind_; }

private:
    Kind kind_;
};

class PointerType : public Type {
public:
    PointerType(const std::shared_ptr<Type> &of, Span span)
        : Type(span), of_(of) {}
    inline void Accept(TypeVisitor &visitor) const override {
        visitor.Visit(*this);
    }
    inline bool IsPointer() const override { return true; }
    inline PointerType *ToPointer() override { return this; }
    inline void Print(PrintableContext &ctx) const override {
        ctx.printer().Print("*");
        of_->Print(ctx);
    }
    const std::shared_ptr<Type> &of() const { return of_; }

private:
    std::shared_ptr<Type> of_;
};

class ArrayType : public Type {
public:
    ArrayType(const std::shared_ptr<Type> &of, std::optional<uint64_t> size,
              Span span)
        : Type(span), of_(of), size_(size) {}
    inline void Accept(TypeVisitor &visitor) const override {
        visitor.Visit(*this);
    }
    inline bool IsArray() const override { return true; }
    inline ArrayType *ToArray() override { return this; }
    void Print(PrintableContext &ctx) const override;
    const std::shared_ptr<Type> &of() const { return of_; }
    inline std::optional<uint64_t> size() const { return size_; }

private:
    std::shared_ptr<Type> of_;
    std::optional<uint64_t> size_;
};

class NameType : public Type {
public:
    NameType(std::string &&value, Span span)
        : Type(span), value_(std::move(value)) {}
    inline void Accept(TypeVisitor &visitor) const override {
        visitor.Visit(*this);
    }
    inline bool IsName() const override { return true; }
    inline NameType *ToName() override { return this; }
    inline void Print(PrintableContext &ctx) const override {
        ctx.printer().Print("{}", value_);
    }
    const std::string &value() const { return value_; }

private:
    std::string value_;
};

}  // namespace hir

}  // namespace mini

#endif  // MINI_HIR_TYPE_H_
