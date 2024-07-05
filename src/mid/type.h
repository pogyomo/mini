#ifndef MINI_MID_TYPE_H_
#define MINI_MID_TYPE_H_

#include <memory>
#include <string>

#include "../span.h"

namespace mid {

class Type {
public:
    Type(Span span) : span_(span) {}
    Span span() const { return span_; }

private:
    Span span_;
};

class IntType : public Type {
    enum Kind {
        I8,
        U8,
        I16,
        U16,
        I32,
        U32,
        I64,
        U64,
    };
    IntType(Kind kind, Span span) : Type(span), kind_(kind) {}
    Kind kind() const { return kind_; }

private:
    Kind kind_;
};

class CharType : public Type {
    CharType(Span span) : Type(span) {}
};

class ArrayType : public Type {
public:
    ArrayType(const std::shared_ptr<Type> &of, uint64_t size, Span span)
        : Type(span), of_(of), size_(size) {}
    const std::shared_ptr<Type> &of() const { return of_; }

private:
    std::shared_ptr<Type> of_;
    uint64_t size_;
};

class PointerType : public Type {
public:
    PointerType(const std::shared_ptr<Type> &of, Span span)
        : Type(span), of_(of) {}
    const std::shared_ptr<Type> &of() const { return of_; }

private:
    std::shared_ptr<Type> of_;
};

class NameType : public Type {
public:
    NameType(std::string &&name, Span span)
        : Type(span), name_(std::move(name)) {}
    const std::string &name() const { return name_; }

private:
    std::string name_;
};

};  // namespace mid

#endif  // MINI_MID_TYPE_H_
