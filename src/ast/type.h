#ifndef MINI_AST_TYPE_H_
#define MINI_AST_TYPE_H_

#include <memory>
#include <string>

#include "node.h"

class Expression;

class IntType;
class UIntType;
class PointerType;
class ArrayType;
class NameType;

class TypeVisitor {
public:
    virtual ~TypeVisitor() {}
    virtual void visit(const IntType& type) = 0;
    virtual void visit(const UIntType& type) = 0;
    virtual void visit(const PointerType& type) = 0;
    virtual void visit(const ArrayType& type) = 0;
    virtual void visit(const NameType& type) = 0;
};

class Type : public Node {
public:
    virtual ~Type() {}
    virtual void accept(TypeVisitor& visitor) const = 0;
};

class IntType : public Type {
public:
    IntType(Span span) : span_(span) {}
    inline void accept(TypeVisitor& visitor) const override {
        visitor.visit(*this);
    }
    inline Span span() const override { return span_; }

private:
    const Span span_;
};

class UIntType : public Type {
public:
    UIntType(Span span) : span_(span) {}
    inline void accept(TypeVisitor& visitor) const override {
        visitor.visit(*this);
    }
    inline Span span() const override { return span_; }

private:
    const Span span_;
};

class PointerType : public Type {
public:
    PointerType(Star star, std::unique_ptr<Type>&& of)
        : star_(star), of_(std::move(of)) {}
    inline void accept(TypeVisitor& visitor) const override {
        visitor.visit(*this);
    }
    inline Span span() const override { return star_.span() + of_->span(); }
    inline Star star() const { return star_; }
    inline const std::unique_ptr<Type>& of() const { return of_; }

private:
    const Star star_;
    const std::unique_ptr<Type> of_;
};

class ArrayType : public Type {
public:
    ArrayType(LParen lparen, std::unique_ptr<Type>&& of, RParen rparen,
              LSquare lsquare, std::unique_ptr<Expression>&& size,
              RSquare rsquare)
        : lparen_(lparen),
          of_(std::move(of)),
          rparen_(rparen),
          lsquare_(lsquare),
          size_(std::move(size)),
          rsquare_(rsquare) {}
    inline void accept(TypeVisitor& visitor) const override {
        visitor.visit(*this);
    }
    inline Span span() const override {
        return lparen_.span() + rsquare_.span();
    }
    inline LParen lparen() const { return lparen_; }
    inline const std::unique_ptr<Type>& of() const { return of_; }
    inline RParen rparen() const { return rparen_; }
    inline LSquare lsquare() const { return lsquare_; }
    inline const std::unique_ptr<Expression>& size() const { return size_; }
    inline RSquare rsquare() const { return rsquare_; }

private:
    const LParen lparen_;
    const std::unique_ptr<Type> of_;
    const RParen rparen_;
    const LSquare lsquare_;
    const std::unique_ptr<Expression> size_;
    const RSquare rsquare_;
};

class NameType : public Type {
public:
    NameType(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline void accept(TypeVisitor& visitor) const override {
        visitor.visit(*this);
    }
    inline Span span() const override { return span_; }

private:
    const std::string name_;
    const Span span_;
};

#endif  // MINI_AST_TYPE_H_
