#ifndef MINI_TOKEN_H_
#define MINI_TOKEN_H_

#include <cstdint>
#include <stdexcept>
#include <string>

#include "span.h"

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
    ColonColon,   // "::"
};

enum class KeywordTokenKind {
    As,        // "as"
    Break,     // "break"
    Continue,  // "continue"
    Else,      // "else"
    If,        // "if"
    Return,    // "return"
    While,     // "while"
    ESizeof,   // "esizeof"
    TSizeof,   // "tsizeof"
    Function,  // "function"
    Struct,    // "struct"
    Enum,      // "enum"
};

class Token {
public:
    Token(Span span) : span_(span) {}
    virtual ~Token() {}
    Span span() const { return span_; }
    virtual bool is_punct_of(PunctTokenKind kind) const { return false; }
    virtual bool is_keyword_of(KeywordTokenKind kind) const { return false; }
    virtual bool is_ident() const { return false; }
    virtual bool is_int() const { return false; }
    virtual const std::string& ident_value() const {
        throw std::runtime_error(
            "`ident_value` called when `is_ident` returns false");
    }
    virtual uint64_t int_value() const {
        throw std::runtime_error(
            "`int_value` called when `is_int` returns false");
    }

private:
    const Span span_;
};

class PunctToken : public Token {
public:
    PunctToken(PunctTokenKind kind, Span span) : Token(span), kind_(kind) {}
    inline bool is_punct_of(PunctTokenKind kind) const override {
        return kind == kind_;
    }

private:
    const PunctTokenKind kind_;
};

class KeywordToken : public Token {
public:
    KeywordToken(KeywordTokenKind kind, Span span) : Token(span), kind_(kind) {}
    inline bool is_keyword_of(KeywordTokenKind kind) const override {
        return kind == kind_;
    }

private:
    const KeywordTokenKind kind_;
};

class IdentToken : public Token {
public:
    IdentToken(std::string&& value, Span span)
        : Token(span), value_(std::move(value)) {}
    inline bool is_ident() const override { return true; }
    inline const std::string& ident_value() const override { return value_; }

private:
    const std::string value_;
};

class IntToken : public Token {
public:
    IntToken(uint64_t value, Span span) : Token(span), value_(value) {}
    inline bool is_int() const override { return true; }
    inline uint64_t int_value() const override { return value_; }

private:
    const uint64_t value_;
};

#endif  // MINI_TOKEN_H_
