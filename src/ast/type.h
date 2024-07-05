#ifndef MINI_AST_TYPE_H_
#define MINI_AST_TYPE_H_

#include <memory>
#include <string>

#include "node.h"

namespace ast {

class Expression;

class BuiltinType;
class PointerType;
class ArrayType;
class NameType;

class TypeVisitor {
public:
    virtual ~TypeVisitor() {}
    virtual void visit(const BuiltinType& type) = 0;
    virtual void visit(const PointerType& type) = 0;
    virtual void visit(const ArrayType& type) = 0;
    virtual void visit(const NameType& type) = 0;
};

class Type : public Node {
public:
    virtual ~Type() {}
    virtual void accept(TypeVisitor& visitor) const = 0;
};

class BuiltinType : public Type {
public:
    enum Kind {
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
    BuiltinType(Kind kind, Span span) : kind_(kind), span_(span) {}
    inline void accept(TypeVisitor& visitor) const override {
        visitor.visit(*this);
    }
    inline Span span() const override { return span_; }
    Kind kind() const { return kind_; }

private:
    Kind kind_;
    Span span_;
};

class PointerType : public Type {
public:
    PointerType(Star star, const std::shared_ptr<Type>& of)
        : star_(star), of_(of) {}
    inline void accept(TypeVisitor& visitor) const override {
        visitor.visit(*this);
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
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

};  // namespace ast

#endif  // MINI_AST_TYPE_H_
