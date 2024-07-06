#include "lexer.h"

#include <cctype>
#include <iterator>
#include <map>
#include <string>

#include "report.h"
#include "span.h"
#include "token.h"

namespace mini {

class LineStream {
public:
    LineStream(size_t row, const std::string &line)
        : offset_(0), row_(row), line_(line) {}
    explicit operator bool() { return !is_eos(); }
    bool is_eos() const { return offset_ >= line_.size(); }
    void advance() { offset_++; }
    char ch() const { return line_[offset_]; }
    Position pos() const { return Position(row_, offset_); }
    bool accept(const std::string &pat, Position &pos) {
        size_t offset_save = offset_;
        Position pos_save = pos;
        for (char c : pat) {
            if (is_eos() || c != ch()) {
                offset_ = offset_save;
                pos = pos_save;
                return false;
            } else {
                pos = this->pos();
                advance();
            }
        }
        return true;
    }
    bool accept(char pat, Position &pos) {
        if (is_eos() || pat != ch()) {
            return false;
        } else {
            pos = this->pos();
            advance();
            return true;
        }
    }
    void skip_spaces() {
        while (!is_eos() && std::isspace(ch())) {
            advance();
        }
    }

private:
    size_t offset_;
    const size_t row_;
    const std::string &line_;
};

static const std::map<std::string, KeywordTokenKind> keywords = {
    {"as", KeywordTokenKind::As},
    {"bool", KeywordTokenKind::Bool},
    {"break", KeywordTokenKind::Break},
    {"char", KeywordTokenKind::Char},
    {"continue", KeywordTokenKind::Continue},
    {"esizeof", KeywordTokenKind::ESizeof},
    {"else", KeywordTokenKind::Else},
    {"enum", KeywordTokenKind::Enum},
    {"false", KeywordTokenKind::False},
    {"function", KeywordTokenKind::Function},
    {"if", KeywordTokenKind::If},
    {"let", KeywordTokenKind::Let},
    {"return", KeywordTokenKind::Return},
    {"struct", KeywordTokenKind::Struct},
    {"tsizeof", KeywordTokenKind::TSizeof},
    {"true", KeywordTokenKind::True},
    {"while", KeywordTokenKind::While},
    {"isize", KeywordTokenKind::ISize},
    {"int8", KeywordTokenKind::Int8},
    {"int16", KeywordTokenKind::Int16},
    {"int32", KeywordTokenKind::Int32},
    {"int64", KeywordTokenKind::Int64},
    {"usize", KeywordTokenKind::USize},
    {"uint8", KeywordTokenKind::UInt8},
    {"uint16", KeywordTokenKind::UInt16},
    {"uint32", KeywordTokenKind::UInt32},
    {"uint64", KeywordTokenKind::UInt64},
};

LexResult lex_line(Context &ctx, size_t id, size_t row,
                   const std::string &line) {
    LineStream stream(row, line);
    bool success = true;
    std::vector<std::unique_ptr<Token>> res;
    while (true) {
        stream.skip_spaces();
        if (!stream) {
            break;
        }

        Position start = stream.pos();
        Position end = start;
        if (stream.accept("+", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::Plus,
                                                       Span(id, start, end)));
        } else if (stream.accept("->", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::Arrow,
                                                       Span(id, start, end)));
        } else if (stream.accept("-", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::Minus,
                                                       Span(id, start, end)));
        } else if (stream.accept("*", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::Star,
                                                       Span(id, start, end)));
        } else if (stream.accept("/", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::Slash,
                                                       Span(id, start, end)));
        } else if (stream.accept("%", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::Percent,
                                                       Span(id, start, end)));
        } else if (stream.accept("||", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::Or,
                                                       Span(id, start, end)));
        } else if (stream.accept("|", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::Vertical,
                                                       Span(id, start, end)));
        } else if (stream.accept("&&", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::And,
                                                       Span(id, start, end)));
        } else if (stream.accept("&", end)) {
            res.push_back(std::make_unique<PunctToken>(
                PunctTokenKind::Ampersand, Span(id, start, end)));
        } else if (stream.accept("^", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::Hat,
                                                       Span(id, start, end)));
        } else if (stream.accept("==", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::EQ,
                                                       Span(id, start, end)));
        } else if (stream.accept("!=", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::NE,
                                                       Span(id, start, end)));
        } else if (stream.accept("=", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::Assign,
                                                       Span(id, start, end)));
        } else if (stream.accept("<=", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::LE,
                                                       Span(id, start, end)));
        } else if (stream.accept("<<", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::LShift,
                                                       Span(id, start, end)));
        } else if (stream.accept("<", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::LT,
                                                       Span(id, start, end)));
        } else if (stream.accept(">=", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::GE,
                                                       Span(id, start, end)));
        } else if (stream.accept(">>", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::RShift,
                                                       Span(id, start, end)));
        } else if (stream.accept(">", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::GT,
                                                       Span(id, start, end)));
        } else if (stream.accept("~", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::Tilde,
                                                       Span(id, start, end)));
        } else if (stream.accept("!", end)) {
            res.push_back(std::make_unique<PunctToken>(
                PunctTokenKind::Exclamation, Span(id, start, end)));
        } else if (stream.accept(".", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::Dot,
                                                       Span(id, start, end)));
        } else if (stream.accept("{", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::LCurly,
                                                       Span(id, start, end)));
        } else if (stream.accept("(", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::LParen,
                                                       Span(id, start, end)));
        } else if (stream.accept("[", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::LSquare,
                                                       Span(id, start, end)));
        } else if (stream.accept("}", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::RCurly,
                                                       Span(id, start, end)));
        } else if (stream.accept(")", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::RParen,
                                                       Span(id, start, end)));
        } else if (stream.accept("]", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::RSquare,
                                                       Span(id, start, end)));
        } else if (stream.accept(";", end)) {
            res.push_back(std::make_unique<PunctToken>(
                PunctTokenKind::Semicolon, Span(id, start, end)));
        } else if (stream.accept(",", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::Comma,
                                                       Span(id, start, end)));
        } else if (stream.accept("::", end)) {
            res.push_back(std::make_unique<PunctToken>(
                PunctTokenKind::ColonColon, Span(id, start, end)));
        } else if (stream.accept(":", end)) {
            res.push_back(std::make_unique<PunctToken>(PunctTokenKind::Colon,
                                                       Span(id, start, end)));
        } else if (stream.accept('"', end)) {
            std::string value;
            while (true) {
                if (!stream) {
                    ReportInfo info(Span(id, start, end),
                                    "unclosing string literal", "");
                    report(ctx, ReportLevel::Error, info);
                    return std::nullopt;
                } else if (stream.accept('"', end)) {
                    break;
                } else if (stream.accept('\\', end)) {
                    if (stream.accept('a', end)) {
                        value.push_back('\a');
                    } else if (stream.accept('b', end)) {
                        value.push_back('\b');
                    } else if (stream.accept('f', end)) {
                        value.push_back('\f');
                    } else if (stream.accept('n', end)) {
                        value.push_back('\n');
                    } else if (stream.accept('r', end)) {
                        value.push_back('\r');
                    } else if (stream.accept('t', end)) {
                        value.push_back('\t');
                    } else if (stream.accept('v', end)) {
                        value.push_back('\v');
                    } else if (stream.accept('\'', end)) {
                        value.push_back('\'');
                    } else if (stream.accept('"', end)) {
                        value.push_back('\"');
                    } else if (stream.accept('\\', end)) {
                        value.push_back('\\');
                    } else {
                        ReportInfo info(Span(id, end, end),
                                        "unexpected escape sequence", "");
                        report(ctx, ReportLevel::Error, info);
                        return std::nullopt;
                    }
                } else {
                    value.push_back(stream.ch());
                    stream.advance();
                }
            }
            res.push_back(std::make_unique<StringToken>(std::move(value),
                                                        Span(id, start, end)));
        } else if (stream.accept('\'', end)) {
            char value;
            if (stream.accept('\\', end)) {
                if (stream.accept('a', end)) {
                    value = '\a';
                } else if (stream.accept('b', end)) {
                    value = '\b';
                } else if (stream.accept('f', end)) {
                    value = '\f';
                } else if (stream.accept('n', end)) {
                    value = '\n';
                } else if (stream.accept('r', end)) {
                    value = '\r';
                } else if (stream.accept('t', end)) {
                    value = '\t';
                } else if (stream.accept('v', end)) {
                    value = '\v';
                } else if (stream.accept('\'', end)) {
                    value = '\'';
                } else if (stream.accept('"', end)) {
                    value = '\"';
                } else if (stream.accept('\\', end)) {
                    value = '\\';
                } else {
                    ReportInfo info(Span(id, end, end),
                                    "unexpected escape sequence", "");
                    report(ctx, ReportLevel::Error, info);
                    return std::nullopt;
                }
            } else if (stream) {
                value = stream.ch();
                end = stream.pos();
                stream.advance();
            } else {
                ReportInfo info(Span(id, end, end),
                                "unclosing character literal", "");
                report(ctx, ReportLevel::Error, info);
                return std::nullopt;
            }

            if (!stream.accept('\'', end)) {
                ReportInfo info(Span(id, end, end),
                                "unclosing character literal", "");
                report(ctx, ReportLevel::Error, info);
                return std::nullopt;
            }

            res.push_back(
                std::make_unique<CharToken>(value, Span(id, start, end)));
        } else if (std::isalpha(stream.ch())) {
            std::string value;
            while (stream) {
                if (std::isalnum(stream.ch()) || stream.ch() == '_') {
                    value.push_back(stream.ch());
                    end = stream.pos();
                    stream.advance();
                } else {
                    break;
                }
            }
            if (keywords.find(value) != keywords.end()) {
                res.push_back(std::make_unique<KeywordToken>(
                    keywords.at(value), Span(id, start, end)));
            } else {
                res.push_back(std::make_unique<IdentToken>(
                    std::move(value), Span(id, start, end)));
            }
        } else if (std::isdigit(stream.ch())) {
            std::string buf;
            while (stream) {
                if (std::isdigit(stream.ch())) {
                    buf.push_back(stream.ch());
                    end = stream.pos();
                    stream.advance();
                } else {
                    break;
                }
            }
            try {
                res.push_back(std::make_unique<IntToken>(std::stoull(buf),
                                                         Span(id, start, end)));
            } catch (std::exception &e) {
                success = false;
                ReportInfo info(Span(id, start, end),
                                "integer convertion failed", "");
                report(ctx, ReportLevel::Error, info);
            }
        } else {
            success = false;
            ReportInfo info(Span(id, start, end), "unexpected character", "");
            report(ctx, ReportLevel::Error, info);
            stream.advance();
        }
    }
    if (success)
        return res;
    else
        return std::nullopt;
}

LexResult lex_file(Context &ctx, const std::string &path) {
    auto id = ctx.input_cache().cache(path);
    auto lines = ctx.input_cache().fetch(id).lines();

    bool success = true;
    std::vector<std::unique_ptr<Token>> res;
    for (size_t row = 0; row < lines.size(); row++) {
        auto line = lex_line(ctx, id, row, lines.at(row));
        if (!line) {
            success = false;
            continue;
        }
        auto begin = std::make_move_iterator(line->begin());
        auto end = std::make_move_iterator(line->end());
        res.insert(res.end(), begin, end);
    }
    if (success)
        return res;
    else
        return std::nullopt;
}

};  // namespace mini
