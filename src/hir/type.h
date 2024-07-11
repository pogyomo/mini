#ifndef MINI_HIR_TYPE_H_
#define MINI_HIR_TYPE_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "../panic.h"
#include "../span.h"
#include "fmt/format.h"
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
    virtual void visit(const BuiltinType &type) = 0;
    virtual void visit(const PointerType &type) = 0;
    virtual void visit(const ArrayType &type) = 0;
    virtual void visit(const NameType &type) = 0;
};

class Type : public Printable {
public:
    Type(Span span) : span_(span) {}
    virtual void accept(TypeVisitor &visitor) const = 0;
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
    inline void accept(TypeVisitor &visitor) const override {
        visitor.visit(*this);
    }
    void print(PrintableContext &ctx) const override {
        if (kind_ == BuiltinType::Void)
            ctx.printer().print("void");
        else if (kind_ == BuiltinType::ISize)
            ctx.printer().print("isize");
        else if (kind_ == BuiltinType::Int8)
            ctx.printer().print("int8");
        else if (kind_ == BuiltinType::Int16)
            ctx.printer().print("int16");
        else if (kind_ == BuiltinType::Int32)
            ctx.printer().print("int32");
        else if (kind_ == BuiltinType::Int64)
            ctx.printer().print("int64");
        else if (kind_ == BuiltinType::USize)
            ctx.printer().print("usize");
        else if (kind_ == BuiltinType::UInt8)
            ctx.printer().print("uint8");
        else if (kind_ == BuiltinType::UInt16)
            ctx.printer().print("uint16");
        else if (kind_ == BuiltinType::UInt32)
            ctx.printer().print("uint32");
        else if (kind_ == BuiltinType::UInt64)
            ctx.printer().print("uint64");
        else if (kind_ == BuiltinType::Char)
            ctx.printer().print("char");
        else if (kind_ == BuiltinType::Bool)
            ctx.printer().print("bool");
        else
            fatal_error("unreachable");
    }
    inline Kind kind() const { return kind_; }

private:
    Kind kind_;
};

class PointerType : public Type {
public:
    PointerType(const std::shared_ptr<Type> &of, Span span)
        : Type(span), of_(of) {}
    inline void accept(TypeVisitor &visitor) const override {
        visitor.visit(*this);
    }
    inline void print(PrintableContext &ctx) const override {
        ctx.printer().print("*");
        of_->print(ctx);
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
    inline void accept(TypeVisitor &visitor) const override {
        visitor.visit(*this);
    }
    void print(PrintableContext &ctx) const override {
        if (size_) {
            ctx.printer().print("(");
            of_->print(ctx);
            ctx.printer().print(fmt::format(")[{}]", size_.value()));
        } else {
            ctx.printer().print("(");
            of_->print(ctx);
            ctx.printer().print(")[]");
        }
    }
    const std::shared_ptr<Type> &of() const { return of_; }

private:
    std::shared_ptr<Type> of_;
    std::optional<uint64_t> size_;
};

class NameType : public Type {
public:
    NameType(std::string &&value, Span span)
        : Type(span), value_(std::move(value)) {}
    inline void accept(TypeVisitor &visitor) const override {
        visitor.visit(*this);
    }
    inline void print(PrintableContext &ctx) const override {
        ctx.printer().print(fmt::format("{}", value_));
    }
    const std::string &value() const { return value_; }

private:
    std::string value_;
};

}  // namespace hir

}  // namespace mini

#endif  // MINI_HIR_TYPE_H_
