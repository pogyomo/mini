#include "token.h"

std::string to_string(PunctTokenKind kind) {
    switch (kind) {
        case PunctTokenKind::Plus:
            return "+";
        case PunctTokenKind::Arrow:
            return "->";
        case PunctTokenKind::Minus:
            return "-";
        case PunctTokenKind::Star:
            return "*";
        case PunctTokenKind::Slash:
            return "/";
        case PunctTokenKind::Percent:
            return "%";
        case PunctTokenKind::Or:
            return "||";
        case PunctTokenKind::Vertical:
            return "|";
        case PunctTokenKind::And:
            return "&&";
        case PunctTokenKind::Ampersand:
            return "&";
        case PunctTokenKind::Hat:
            return "^";
        case PunctTokenKind::EQ:
            return "==";
        case PunctTokenKind::NE:
            return "!=";
        case PunctTokenKind::Assign:
            return "=";
        case PunctTokenKind::LE:
            return "<=";
        case PunctTokenKind::LShift:
            return "<<";
        case PunctTokenKind::LT:
            return "<";
        case PunctTokenKind::GE:
            return ">=";
        case PunctTokenKind::RShift:
            return ">>";
        case PunctTokenKind::GT:
            return ">";
        case PunctTokenKind::Tilde:
            return "~";
        case PunctTokenKind::Exclamation:
            return "!";
        case PunctTokenKind::Dot:
            return ".";
        case PunctTokenKind::LCurly:
            return "{";
        case PunctTokenKind::LParen:
            return "(";
        case PunctTokenKind::LSquare:
            return "[";
        case PunctTokenKind::RCurly:
            return "}";
        case PunctTokenKind::RParen:
            return ")";
        case PunctTokenKind::RSquare:
            return "]";
        case PunctTokenKind::Semicolon:
            return ";";
        case PunctTokenKind::Comma:
            return ",";
        case PunctTokenKind::Colon:
            return ":";
        case PunctTokenKind::ColonColon:
            return "::";
        default:
            return "";
    }
}

std::string to_string(KeywordTokenKind kind) {
    switch (kind) {
        case KeywordTokenKind::As:
            return "as";
        case KeywordTokenKind::Break:
            return "break";
        case KeywordTokenKind::Continue:
            return "continue";
        case KeywordTokenKind::ESizeof:
            return "esizeof";
        case KeywordTokenKind::Else:
            return "else";
        case KeywordTokenKind::Enum:
            return "enum";
        case KeywordTokenKind::Function:
            return "function";
        case KeywordTokenKind::If:
            return "if";
        case KeywordTokenKind::Int:
            return "int";
        case KeywordTokenKind::Let:
            return "let";
        case KeywordTokenKind::Return:
            return "return";
        case KeywordTokenKind::Struct:
            return "struct";
        case KeywordTokenKind::TSizeof:
            return "tsizeof";
        case KeywordTokenKind::UInt:
            return "uint";
        case KeywordTokenKind::While:
            return "while";
        default:
            return "";
    }
}
