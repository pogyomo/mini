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
};

std::string to_string(PunctTokenKind kind);
std::string to_string(KeywordTokenKind kind);

class Token {
public:
    Token(Span span) : span_(span) {}
    virtual ~Token() {}
    Span span() const { return span_; }
    virtual bool is_punct_of(PunctTokenKind kind) const { return false; }
    virtual bool is_keyword_of(KeywordTokenKind kind) const { return false; }
    virtual bool is_ident() const { return false; }
    virtual bool is_int() const { return false; }
    virtual bool is_string() const { return false; }
    virtual bool is_char() const { return false; }
    virtual const std::string& ident_value() const {
        throw std::runtime_error(
            "`ident_value` called when `is_ident` returns false");
    }
    virtual uint64_t int_value() const {
        throw std::runtime_error(
            "`int_value` called when `is_int` returns false");
    }
    virtual const std::string& string_value() const {
        throw std::runtime_error(
            "`string_value` called when `is_string` returns false");
    }
    virtual char char_value() const {
        throw std::runtime_error(
            "`char_value` called when `is_char` returns false");
    }

private:
    Span span_;
};

class PunctToken : public Token {
public:
    PunctToken(PunctTokenKind kind, Span span) : Token(span), kind_(kind) {}
    inline bool is_punct_of(PunctTokenKind kind) const override {
        return kind == kind_;
    }

private:
    PunctTokenKind kind_;
};

class KeywordToken : public Token {
public:
    KeywordToken(KeywordTokenKind kind, Span span) : Token(span), kind_(kind) {}
    inline bool is_keyword_of(KeywordTokenKind kind) const override {
        return kind == kind_;
    }

private:
    KeywordTokenKind kind_;
};

class IdentToken : public Token {
public:
    IdentToken(std::string&& value, Span span)
        : Token(span), value_(std::move(value)) {}
    inline bool is_ident() const override { return true; }
    inline const std::string& ident_value() const override { return value_; }

private:
    std::string value_;
};

class IntToken : public Token {
public:
    IntToken(uint64_t value, Span span) : Token(span), value_(value) {}
    inline bool is_int() const override { return true; }
    inline uint64_t int_value() const override { return value_; }

private:
    uint64_t value_;
};

class StringToken : public Token {
public:
    StringToken(std::string&& value, Span span)
        : Token(span), value_(std::move(value)) {}
    inline bool is_string() const override { return true; }
    inline const std::string& string_value() const override { return value_; }

private:
    std::string value_;
};

class CharToken : public Token {
public:
    CharToken(char value, Span span) : Token(span), value_(value) {}
    inline bool is_char() const override { return true; }
    inline char char_value() const override { return value_; }

private:
    char value_;
};

};  // namespace mini

#endif  // MINI_TOKEN_H_
