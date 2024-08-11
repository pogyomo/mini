#ifndef MINI_TOKEN_H_
#define MINI_TOKEN_H_

#include <cstdint>
#include <stdexcept>
#include <string>

#include "span.h"

namespace mini {

enum class PunctTokenKind {
    Plus,         // "+"
    Arrow,        // "->"
    Minus,        // "-"
    Star,         // "*"
    Slash,        // "/"
    Percent,      // "%"
    Or,           // "||"
    Vertical,     // "|"
    And,          // "&&"
    Ampersand,    // "&"
    Hat,          // "^"
    EQ,           // "=="
    NE,           // "!="
    Assign,       // "="
    LE,           // "<="
    LShift,       // "<<"
    LT,           // "<"
    GE,           // ">="
    RShift,       // ">>"
    GT,           // ">"
    Tilde,        // "~"
    Exclamation,  // "!"
    Dot,          // "."
    LCurly,       // "{"
    LParen,       // "("
    LSquare,      // "["
    RCurly,       // "}"
    RParen,       // ")"
    RSquare,      // "]"
    Semicolon,    // ";"
    Comma,        // ","
    Colon,        // ":"
    ColonColon,   // "::"
    DotDotDot,    // "..."
};

enum class KeywordTokenKind {
    As,        // "as"
    Bool,      // "bool"
    Break,     // "break"
    Char,      // "char"
    Continue,  // "continue"
    ESizeof,   // "esizeof"
    Else,      // "else"
    Enum,      // "enum"
    False,     // "false"
    Function,  // "function"
    If,        // "if"
    Let,       // "let"
    Return,    // "return"
    Struct,    // "struct"
    TSizeof,   // "tsizeof"
    True,      // "true"
    While,     // "while"
    Void,      // "void"
    ISize,     // "isize"
    Int8,      // "int8"
    Int16,     // "int16"
    Int32,     // "int32"
    Int64,     // "int64"
    USize,     // "usize"
    UInt8,     // "uint8"
    UInt16,    // "uint16"
    UInt32,    // "uint32"
    UInt64,    // "uint64"
    NullPtr,   // "nullptr"
};

std::string ToString(PunctTokenKind kind);
std::string ToString(KeywordTokenKind kind);

class Token {
public:
    Token(Span span) : span_(span) {}
    virtual ~Token() {}
    Span span() const { return span_; }
    virtual bool IsPunctOf(PunctTokenKind) const { return false; }
    virtual bool IsKeywordOf(KeywordTokenKind) const { return false; }
    virtual bool IsIdent() const { return false; }
    virtual bool IsInt() const { return false; }
    virtual bool IsString() const { return false; }
    virtual bool IsChar() const { return false; }
    virtual const std::string& IdentValue() const {
        throw std::runtime_error(
            "`IdentValue` called when `IsIdent` returns false");
    }
    virtual uint64_t IntValue() const {
        throw std::runtime_error(
            "`IntValue` called when `IsInt` returns false");
    }
    virtual const std::string& StringValue() const {
        throw std::runtime_error(
            "`StringValue` called when `IsString` returns false");
    }
    virtual char CharValue() const {
        throw std::runtime_error(
            "`CharValue` called when `IsChar` returns false");
    }

private:
    Span span_;
};

class PunctToken : public Token {
public:
    PunctToken(PunctTokenKind kind, Span span) : Token(span), kind_(kind) {}
    inline bool IsPunctOf(PunctTokenKind kind) const override {
        return kind == kind_;
    }

private:
    PunctTokenKind kind_;
};

class KeywordToken : public Token {
public:
    KeywordToken(KeywordTokenKind kind, Span span) : Token(span), kind_(kind) {}
    inline bool IsKeywordOf(KeywordTokenKind kind) const override {
        return kind == kind_;
    }

private:
    KeywordTokenKind kind_;
};

class IdentToken : public Token {
public:
    IdentToken(std::string&& value, Span span)
        : Token(span), value_(std::move(value)) {}
    inline bool IsIdent() const override { return true; }
    inline const std::string& IdentValue() const override { return value_; }

private:
    std::string value_;
};

class IntToken : public Token {
public:
    IntToken(uint64_t value, Span span) : Token(span), value_(value) {}
    inline bool IsInt() const override { return true; }
    inline uint64_t IntValue() const override { return value_; }

private:
    uint64_t value_;
};

class StringToken : public Token {
public:
    StringToken(std::string&& value, Span span)
        : Token(span), value_(std::move(value)) {}
    inline bool IsString() const override { return true; }
    inline const std::string& StringValue() const override { return value_; }

private:
    std::string value_;
};

class CharToken : public Token {
public:
    CharToken(char value, Span span) : Token(span), value_(value) {}
    inline bool IsChar() const override { return true; }
    inline char CharValue() const override { return value_; }

private:
    char value_;
};

};  // namespace mini

#endif  // MINI_TOKEN_H_
