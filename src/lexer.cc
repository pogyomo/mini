#include "lexer.h"

#include <cctype>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "context.h"
#include "panic.h"
#include "report.h"
#include "span.h"
#include "token.h"

namespace mini {

class LineStream {
public:
    LineStream(size_t row, const std::string &line)
        : offset_(0), row_(row), line_(line) {}
    explicit operator bool() { return offset_ < line_.size(); }
    void Advance() { offset_++; }
    char Char() const { return line_[offset_]; }
    Position Pos() const { return Position(row_, offset_); }
    bool Accept(const std::string &pat, Position &pos) {
        size_t offset_save = offset_;
        Position pos_save = pos;
        for (char c : pat) {
            if (!*this || c != Char()) {
                offset_ = offset_save;
                pos = pos_save;
                return false;
            } else {
                pos = this->Pos();
                Advance();
            }
        }
        return true;
    }
    bool Accept(char pat, Position &pos) {
        if (!*this || pat != Char()) {
            return false;
        } else {
            pos = this->Pos();
            Advance();
            return true;
        }
    }
    void SkipSpaces() {
        while (*this && std::isspace(Char())) {
            Advance();
        }
    }

private:
    size_t offset_;
    const size_t row_;
    const std::string &line_;
};

static const std::map<std::string, KeywordTokenKind> keywords = {
    {"as",       KeywordTokenKind::As      },
    {"bool",     KeywordTokenKind::Bool    },
    {"break",    KeywordTokenKind::Break   },
    {"char",     KeywordTokenKind::Char    },
    {"continue", KeywordTokenKind::Continue},
    {"esizeof",  KeywordTokenKind::ESizeof },
    {"else",     KeywordTokenKind::Else    },
    {"enum",     KeywordTokenKind::Enum    },
    {"false",    KeywordTokenKind::False   },
    {"function", KeywordTokenKind::Function},
    {"if",       KeywordTokenKind::If      },
    {"let",      KeywordTokenKind::Let     },
    {"return",   KeywordTokenKind::Return  },
    {"struct",   KeywordTokenKind::Struct  },
    {"tsizeof",  KeywordTokenKind::TSizeof },
    {"true",     KeywordTokenKind::True    },
    {"while",    KeywordTokenKind::While   },
    {"void",     KeywordTokenKind::Void    },
    {"isize",    KeywordTokenKind::ISize   },
    {"int8",     KeywordTokenKind::Int8    },
    {"int16",    KeywordTokenKind::Int16   },
    {"int32",    KeywordTokenKind::Int32   },
    {"int64",    KeywordTokenKind::Int64   },
    {"usize",    KeywordTokenKind::USize   },
    {"uint8",    KeywordTokenKind::UInt8   },
    {"uint16",   KeywordTokenKind::UInt16  },
    {"uint32",   KeywordTokenKind::UInt32  },
    {"uint64",   KeywordTokenKind::UInt64  },
    {"nullptr",  KeywordTokenKind::NullPtr },
};

static const std::vector<std::pair<std::string, PunctTokenKind>> puncts = {
    {"+",   PunctTokenKind::Plus       },
    {"->",  PunctTokenKind::Arrow      },
    {"-",   PunctTokenKind::Minus      },
    {"*",   PunctTokenKind::Star       },
    {"/",   PunctTokenKind::Slash      },
    {"%",   PunctTokenKind::Percent    },
    {"||",  PunctTokenKind::Or         },
    {"|",   PunctTokenKind::Vertical   },
    {"&&",  PunctTokenKind::And        },
    {"&",   PunctTokenKind::Ampersand  },
    {"^",   PunctTokenKind::Hat        },
    {"==",  PunctTokenKind::EQ         },
    {"!=",  PunctTokenKind::NE         },
    {"=",   PunctTokenKind::Assign     },
    {"<=",  PunctTokenKind::LE         },
    {"<<",  PunctTokenKind::LShift     },
    {"<",   PunctTokenKind::LT         },
    {">=",  PunctTokenKind::GE         },
    {">>",  PunctTokenKind::RShift     },
    {">",   PunctTokenKind::GT         },
    {"~",   PunctTokenKind::Tilde      },
    {"!",   PunctTokenKind::Exclamation},
    {".",   PunctTokenKind::Dot        },
    {"...", PunctTokenKind::DotDotDot  },
    {"{",   PunctTokenKind::LCurly     },
    {"(",   PunctTokenKind::LParen     },
    {"[",   PunctTokenKind::LSquare    },
    {"}",   PunctTokenKind::RCurly     },
    {")",   PunctTokenKind::RParen     },
    {"]",   PunctTokenKind::RSquare    },
    {";",   PunctTokenKind::Semicolon  },
    {",",   PunctTokenKind::Comma      },
    {":",   PunctTokenKind::Colon      },
    {"::",  PunctTokenKind::ColonColon },
};

class LexContext {
public:
    LexContext(Context &ctx) : ctx_(ctx), multiline_comment_depth_(0) {}
    Context &ctx() { return ctx_; }
    bool InsideOfMultilineComment() { return multiline_comment_depth_ != 0; }
    void EnterMultilineComment() { multiline_comment_depth_++; }
    void LeaveMultilineComment() {
        if (multiline_comment_depth_) {
            multiline_comment_depth_--;
        } else {
            FatalError("leave from outside of multiline comment");
        }
    }

private:
    Context &ctx_;
    uint64_t multiline_comment_depth_;
};

static LexResult lex_line(LexContext &ctx, size_t id, size_t row,
                          const std::string &line) {
    LineStream stream(row, line);
    bool success = true;
    std::vector<std::unique_ptr<Token>> res;
    while (true) {
        Position tmp(0, 0);
        while (stream && ctx.InsideOfMultilineComment()) {
            if (stream.Accept("*/", tmp)) {
                ctx.LeaveMultilineComment();
            } else if (stream.Accept("/*", tmp)) {
                ctx.EnterMultilineComment();
            } else {
                stream.Advance();
            }
        }

        stream.SkipSpaces();
        if (!stream) {
            break;
        }

        Position start = stream.Pos();
        Position end = start;

        if (stream.Accept("//", end)) {
            break;
        } else if (stream.Accept("/*", end)) {
            ctx.EnterMultilineComment();
            continue;
        }

        bool found = false;
        for (const auto &punct : puncts) {
            if (stream.Accept(punct.first, end)) {
                res.push_back(std::make_unique<PunctToken>(
                    punct.second, Span(id, start, end)));
                found = true;
                break;
            }
        }
        if (found) continue;

        if (stream.Accept('"', end)) {
            std::string value;
            while (true) {
                if (!stream) {
                    ReportInfo info(Span(id, start, end),
                                    "unclosing string literal", "");
                    Report(ctx.ctx(), ReportLevel::Error, info);
                    return std::nullopt;
                } else if (stream.Accept('"', end)) {
                    break;
                } else if (stream.Accept('\\', end)) {
                    if (stream.Accept('a', end)) {
                        value.push_back('\a');
                    } else if (stream.Accept('b', end)) {
                        value.push_back('\b');
                    } else if (stream.Accept('f', end)) {
                        value.push_back('\f');
                    } else if (stream.Accept('n', end)) {
                        value.push_back('\n');
                    } else if (stream.Accept('r', end)) {
                        value.push_back('\r');
                    } else if (stream.Accept('t', end)) {
                        value.push_back('\t');
                    } else if (stream.Accept('v', end)) {
                        value.push_back('\v');
                    } else if (stream.Accept('\'', end)) {
                        value.push_back('\'');
                    } else if (stream.Accept('"', end)) {
                        value.push_back('\"');
                    } else if (stream.Accept('\\', end)) {
                        value.push_back('\\');
                    } else if (stream.Accept('0', end)) {
                        value.push_back('\0');
                    } else {
                        ReportInfo info(Span(id, end, end),
                                        "unexpected escape sequence", "");
                        Report(ctx.ctx(), ReportLevel::Error, info);
                        return std::nullopt;
                    }
                } else {
                    value.push_back(stream.Char());
                    stream.Advance();
                }
            }
            res.push_back(std::make_unique<StringToken>(std::move(value),
                                                        Span(id, start, end)));
        } else if (stream.Accept('\'', end)) {
            char value;
            if (stream.Accept('\\', end)) {
                if (stream.Accept('a', end)) {
                    value = '\a';
                } else if (stream.Accept('b', end)) {
                    value = '\b';
                } else if (stream.Accept('f', end)) {
                    value = '\f';
                } else if (stream.Accept('n', end)) {
                    value = '\n';
                } else if (stream.Accept('r', end)) {
                    value = '\r';
                } else if (stream.Accept('t', end)) {
                    value = '\t';
                } else if (stream.Accept('v', end)) {
                    value = '\v';
                } else if (stream.Accept('\'', end)) {
                    value = '\'';
                } else if (stream.Accept('"', end)) {
                    value = '\"';
                } else if (stream.Accept('\\', end)) {
                    value = '\\';
                } else if (stream.Accept('0', end)) {
                    value = '\0';
                } else {
                    ReportInfo info(Span(id, end, end),
                                    "unexpected escape sequence", "");
                    Report(ctx.ctx(), ReportLevel::Error, info);
                    return std::nullopt;
                }
            } else if (stream) {
                value = stream.Char();
                end = stream.Pos();
                stream.Advance();
            } else {
                ReportInfo info(Span(id, end, end),
                                "unclosing character literal", "");
                Report(ctx.ctx(), ReportLevel::Error, info);
                return std::nullopt;
            }

            if (!stream.Accept('\'', end)) {
                ReportInfo info(Span(id, end, end),
                                "unclosing character literal", "");
                Report(ctx.ctx(), ReportLevel::Error, info);
                return std::nullopt;
            }

            res.push_back(
                std::make_unique<CharToken>(value, Span(id, start, end)));
        } else if (std::isalpha(stream.Char())) {
            std::string value;
            while (stream) {
                if (std::isalnum(stream.Char()) || stream.Char() == '_') {
                    value.push_back(stream.Char());
                    end = stream.Pos();
                    stream.Advance();
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
        } else if (std::isdigit(stream.Char())) {
            std::string buf;
            while (stream) {
                if (std::isdigit(stream.Char())) {
                    buf.push_back(stream.Char());
                    end = stream.Pos();
                    stream.Advance();
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
                Report(ctx.ctx(), ReportLevel::Error, info);
            }
        } else {
            success = false;
            ReportInfo info(Span(id, start, end), "unexpected character", "");
            Report(ctx.ctx(), ReportLevel::Error, info);
            stream.Advance();
        }
    }
    if (success)
        return res;
    else
        return std::nullopt;
}

LexResult LexFile(Context &ctx, const std::string &path) {
    auto id = ctx.input_cache().Cache(path);
    auto lines = ctx.input_cache().Fetch(id).lines();

    bool success = true;
    std::vector<std::unique_ptr<Token>> res;
    LexContext lex_ctx(ctx);
    for (size_t row = 0; row < lines.size(); row++) {
        auto line = lex_line(lex_ctx, id, row, lines.at(row));
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
