#ifndef MINI_AST_TYPE_H_
#define MINI_AST_TYPE_H_

#include <memory>
#include <string>

#include "node.h"

namespace ast {

class Expression;

class IntType;
class UIntType;
class CharType;
class BoolType;
class PointerType;
class ArrayType;
class NameType;

class TypeVisitor {
public:
    virtual ~TypeVisitor() {}
    virtual void visit(const IntType& type) = 0;
    virtual void visit(const UIntType& type) = 0;
    virtual void visit(const BoolType& type) = 0;
    virtual void visit(const CharType& type) = 0;
    virtual void visit(const PointerType& type) = 0;
    virtual void visit(const ArrayType& type) = 0;
    virtual void visit(const NameType& type) = 0;
};

enum class TypeKind {
    UnitType,
    IntType,
    UIntType,
    CharType,
    BoolType,
    PointerType,
    ArrayType,
    NameType,
};

class Type : public Node {
public:
    virtual ~Type() {}
    virtual void accept(TypeVisitor& visitor) const = 0;
    virtual bool is(TypeKind kind) const = 0;
    virtual bool operator==(const Type& other) const = 0;
    bool operator!=(const Type& other) const { return !(*this == other); }
};

class IntType : public Type {
public:
    IntType(Span span) : span_(span) {}
    inline void accept(TypeVisitor& visitor) const override {
        visitor.visit(*this);
    }
    bool is(TypeKind kind) const override { return kind == TypeKind::IntType; }
    bool operator==(const Type& other) const override {
        return other.is(TypeKind::IntType);
    }
    inline Span span() const override { return span_; }

private:
    Span span_;
};

class UIntType : public Type {
public:
    UIntType(Span span) : span_(span) {}
    inline void accept(TypeVisitor& visitor) const override {
        visitor.visit(*this);
    }
    bool is(TypeKind kind) const override { return kind == TypeKind::UIntType; }
    bool operator==(const Type& other) const override {
        return other.is(TypeKind::UIntType);
    }
    inline Span span() const override { return span_; }

private:
    Span span_;
};

class BoolType : public Type {
public:
    BoolType(Span span) : span_(span) {}
    inline void accept(TypeVisitor& visitor) const override {
        visitor.visit(*this);
    }
    bool is(TypeKind kind) const override { return kind == TypeKind::BoolType; }
    bool operator==(const Type& other) const override {
        return other.is(TypeKind::BoolType);
    }
    inline Span span() const override { return span_; }

private:
    Span span_;
};

class CharType : public Type {
public:
    CharType(Span span) : span_(span) {}
    inline void accept(TypeVisitor& visitor) const override {
        visitor.visit(*this);
    }
    bool is(TypeKind kind) const override { return kind == TypeKind::CharType; }
    bool operator==(const Type& other) const override {
        return other.is(TypeKind::CharType);
    }
    inline Span span() const override { return span_; }

private:
    Span span_;
};

class PointerType : public Type {
public:
    PointerType(Star star, const std::shared_ptr<Type>& of)
        : star_(star), of_(of) {}
    inline void accept(TypeVisitor& visitor) const override {
        visitor.visit(*this);
    }
    bool is(TypeKind kind) const override {
        return kind == TypeKind::PointerType;
    }
    bool operator==(const Type& other) const override {
        if (other.is(TypeKind::PointerType)) {
            return *this->of_ == *static_cast<const PointerType*>(&other);
        } else {
            return false;
        }
    }
    inline Span span() const override { return star_.span() + of_->span(); }
    inline Star star() const { return star_; }
    inline const std::shared_ptr<Type>& of() const { return of_; }

private:
    Star star_;
    std::shared_ptr<Type> of_;
};

class ArrayType : public Type {
public:
    ~ArrayType();
    ArrayType(LParen lparen, const std::shared_ptr<Type>& of, RParen rparen,
              LSquare lsquare, std::unique_ptr<Expression>&& size,
              RSquare rsquare);
    inline void accept(TypeVisitor& visitor) const override {
        visitor.visit(*this);
    }
    bool is(TypeKind kind) const override {
        return kind == TypeKind::ArrayType;
    }
    bool operator==(const Type& other) const override {
        if (other.is(TypeKind::ArrayType)) {
            return *this->of_ == *static_cast<const ArrayType*>(&other);
        } else {
            return false;
        }
    }
    inline Span span() const override {
        return lparen_.span() + rsquare_.span();
    }
    inline LParen lparen() const { return lparen_; }
    inline const std::shared_ptr<Type>& of() const { return of_; }
    inline RParen rparen() const { return rparen_; }
    inline LSquare lsquare() const { return lsquare_; }
    inline const std::unique_ptr<Expression>& size() const { return size_; }
    inline RSquare rsquare() const { return rsquare_; }

private:
    LParen lparen_;
    std::shared_ptr<Type> of_;
    RParen rparen_;
    LSquare lsquare_;
    std::unique_ptr<Expression> size_;
    RSquare rsquare_;
};

class NameType : public Type {
public:
    NameType(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline void accept(TypeVisitor& visitor) const override {
        visitor.visit(*this);
    }
    bool is(TypeKind kind) const override { return kind == TypeKind::NameType; }
    bool operator==(const Type& other) const override {
        if (other.is(TypeKind::NameType)) {
            return this->name_ == static_cast<const NameType*>(&other)->name_;
        } else {
            return false;
        }
    }
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

};  // namespace ast

#endif  // MINI_AST_TYPE_H_
