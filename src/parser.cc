#include "parser.h"

#include <memory>
#include <optional>
#include <vector>

#include "ast/expr.h"
#include "ast/node.h"
#include "ast/stmt.h"
#include "ast/type.h"
#include "context.h"
#include "lexer.h"
#include "report.h"
#include "token.h"

namespace mini {

#define TRY(cond)            \
    if ((cond)) {            \
        return std::nullopt; \
    }

// Returns true if `ts` reaches to eos, and report it.
bool check_eos(Context &ctx, TokenStream &ts) {
    if (ts.is_eos()) {
        ReportInfo info(ts.last()->span(), "expected token after this", "");
        report(ctx, ReportLevel::Error, info);
        return true;
    } else {
        return false;
    }
}

// Returns true if current token in `ts` is not ident, and report it.
bool check_ident(Context &ctx, TokenStream &ts) {
    if (check_eos(ctx, ts)) {
        return true;
    } else {
        if (!ts.token()->is_ident()) {
            if (ts.has_prev()) {
                ReportInfo info(ts.prev()->span(),
                                "expected identifier after this", "");
                report(ctx, ReportLevel::Error, info);
                return true;
            } else {
                ReportInfo info(ts.token()->span(),
                                "expected this to be identifier", "");
                report(ctx, ReportLevel::Error, info);
                return true;
            }
        } else {
            return false;
        }
    }
}

// Returns true if current token in `ts` is not `kind`, and report it.
bool check_punct(Context &ctx, TokenStream &ts, PunctTokenKind kind) {
    if (check_eos(ctx, ts)) {
        return true;
    } else {
        if (!ts.token()->is_punct_of(kind)) {
            if (ts.has_prev()) {
                ReportInfo info(ts.prev()->span(),
                                "expected `" + to_string(kind) + "` after this",
                                "");
                report(ctx, ReportLevel::Error, info);
                return true;
            } else {
                ReportInfo info(ts.token()->span(),
                                "expected this to be `" + to_string(kind) + "`",
                                "");
                report(ctx, ReportLevel::Error, info);
                return true;
            }
        } else {
            return false;
        }
    }
}

// Returns true if current token in `ts` is not `kind`, and report it.
bool check_keyword(Context &ctx, TokenStream &ts, KeywordTokenKind kind) {
    if (check_eos(ctx, ts)) {
        return true;
    } else {
        if (!ts.token()->is_keyword_of(kind)) {
            if (ts.has_prev()) {
                ReportInfo info(ts.prev()->span(),
                                "expected `" + to_string(kind) + "` after this",
                                "");
                report(ctx, ReportLevel::Error, info);
                return true;
            } else {
                ReportInfo info(ts.token()->span(),
                                "expected this to be `" + to_string(kind) + "`",
                                "");
                report(ctx, ReportLevel::Error, info);
                return true;
            }
        } else {
            return false;
        }
    }
}

std::optional<std::unique_ptr<ast::Expression>> parse_expr(Context &ctx,
                                                           TokenStream &ts) {
    auto state = ts.state();
    ctx.enable_suppress_report();

    auto lhs = parse_unary_expr(ctx, ts);
    if (!lhs) {
        ts.set_state(state);
        ctx.disable_suppress_report();
        return parse_logical_or_expr(ctx, ts);
    }

    if (ts.is_eos() || !ts.token()->is_punct_of(PunctTokenKind::Assign)) {
        ts.set_state(state);
        ctx.disable_suppress_report();
        return parse_logical_or_expr(ctx, ts);
    }
    ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::Assign,
                                ts.token()->span());
    ts.advance();

    ctx.disable_suppress_report();
    auto rhs = parse_expr(ctx, ts);
    if (!rhs) return std::nullopt;

    return std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                  std::move(*rhs));
}

std::optional<std::unique_ptr<ast::Expression>> parse_logical_or_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_logical_and_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::Or)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::Or,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_logical_or_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> parse_logical_and_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_inclusive_or_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::And)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::And,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_logical_and_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> parse_inclusive_or_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_exclusive_or_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::Vertical)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::BitOr,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_inclusive_or_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> parse_exclusive_or_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_and_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::Hat)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::BitXor,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_exclusive_or_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> parse_and_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_equality_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::Ampersand)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::BitAnd,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_and_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> parse_equality_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_relational_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::EQ)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::EQ,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_equality_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::NE)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::NE,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_equality_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> parse_relational_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_shift_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::LT)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::LT,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_relational_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::LE)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::LE,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_relational_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::GT)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::GT,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_relational_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::GE)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::GE,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_relational_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> parse_shift_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_additive_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::LShift)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::LShift,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_shift_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::RShift)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::RShift,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_shift_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> parse_additive_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_multiplicative_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::Plus)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::Add,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_additive_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::Minus)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::Sub,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_additive_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> parse_multiplicative_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_cast_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::Star)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::Mul,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_multiplicative_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::Slash)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::Div,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_multiplicative_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::Percent)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::Mod,
                                        ts.token()->span());
            ts.advance();
            auto rhs = parse_multiplicative_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> parse_cast_expr(
    Context &ctx, TokenStream &ts) {
    auto expr = parse_unary_expr(ctx, ts);
    if (!expr) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_keyword_of(KeywordTokenKind::As)) {
            ast::As as_kw(ts.token()->span());
            ts.advance();

            auto type = parse_type(ctx, ts);
            if (!type) return std::nullopt;

            expr = std::make_unique<ast::CastExpression>(
                std::move(*expr), as_kw, std::move(*type));
        } else {
            break;
        }
    }
    return expr;
}

std::optional<std::unique_ptr<ast::Expression>> parse_unary_expr(
    Context &ctx, TokenStream &ts) {
    TRY(check_eos(ctx, ts));
    if (ts.token()->is_punct_of(PunctTokenKind::Ampersand)) {
        ast::UnaryExpression::Op op(ast::UnaryExpression::Op::Kind::Ref,
                                    ts.token()->span());
        ts.advance();

        auto expr = parse_unary_expr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<ast::UnaryExpression>(op, std::move(*expr));
    } else if (ts.token()->is_punct_of(PunctTokenKind::Star)) {
        ast::UnaryExpression::Op op(ast::UnaryExpression::Op::Kind::Deref,
                                    ts.token()->span());
        ts.advance();

        auto expr = parse_unary_expr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<ast::UnaryExpression>(op, std::move(*expr));
    } else if (ts.token()->is_punct_of(PunctTokenKind::Minus)) {
        ast::UnaryExpression::Op op(ast::UnaryExpression::Op::Kind::Minus,
                                    ts.token()->span());
        ts.advance();

        auto expr = parse_unary_expr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<ast::UnaryExpression>(op, std::move(*expr));
    } else if (ts.token()->is_punct_of(PunctTokenKind::Tilde)) {
        ast::UnaryExpression::Op op(ast::UnaryExpression::Op::Kind::Inv,
                                    ts.token()->span());
        ts.advance();

        auto expr = parse_unary_expr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<ast::UnaryExpression>(op, std::move(*expr));
    } else if (ts.token()->is_punct_of(PunctTokenKind::Exclamation)) {
        ast::UnaryExpression::Op op(ast::UnaryExpression::Op::Kind::Neg,
                                    ts.token()->span());
        ts.advance();

        auto expr = parse_unary_expr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<ast::UnaryExpression>(op, std::move(*expr));
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::ESizeof)) {
        ast::ESizeof esizeof_kw(ts.token()->span());
        ts.advance();

        auto expr = parse_expr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<ast::ESizeofExpression>(esizeof_kw,
                                                        std::move(*expr));
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::TSizeof)) {
        ast::TSizeof tsizeof_kw(ts.token()->span());
        ts.advance();

        auto type = parse_type(ctx, ts);
        if (!type) return std::nullopt;

        return std::make_unique<ast::TSizeofExpression>(tsizeof_kw,
                                                        std::move(*type));
    } else {
        return parse_postfix_expr(ctx, ts);
    }
}

std::optional<std::unique_ptr<ast::Expression>> parse_postfix_expr(
    Context &ctx, TokenStream &ts) {
    auto expr = parse_primary_expr(ctx, ts);
    if (!expr) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::LSquare)) {
            ast::LSquare lsquare(ts.token()->span());
            ts.advance();

            auto index = parse_expr(ctx, ts);
            if (!index) return std::nullopt;

            TRY(check_punct(ctx, ts, PunctTokenKind::RSquare));
            ast::RSquare rsquare(ts.token()->span());
            ts.advance();

            expr = std::make_unique<ast::IndexExpression>(
                std::move(*expr), lsquare, std::move(*index), rsquare);
        } else if (ts.token()->is_punct_of(PunctTokenKind::LParen)) {
            ast::LParen lparen(ts.token()->span());
            ts.advance();

            std::vector<std::unique_ptr<ast::Expression>> args;
            while (true) {
                TRY(check_eos(ctx, ts));
                if (ts.token()->is_punct_of(PunctTokenKind::RParen) &&
                    args.empty()) {
                    ast::RParen rparen(ts.token()->span());
                    ts.advance();

                    expr = std::make_unique<ast::CallExpression>(
                        std::move(*expr), lparen, std::move(args), rparen);
                    break;
                } else {
                    auto arg = parse_expr(ctx, ts);
                    if (!arg) return std::nullopt;

                    args.emplace_back(std::move(*arg));

                    if (ts.token()->is_punct_of(PunctTokenKind::RParen)) {
                        ast::RParen rparen(ts.token()->span());
                        ts.advance();

                        expr = std::make_unique<ast::CallExpression>(
                            std::move(*expr), lparen, std::move(args), rparen);
                        break;
                    } else if (ts.token()->is_punct_of(PunctTokenKind::Comma)) {
                        ts.advance();
                        continue;
                    } else {
                        ReportInfo info(ts.token()->span(), "unexpected token",
                                        "expected `)` or `,`");
                        report(ctx, ReportLevel::Error, info);
                        return std::nullopt;
                    }
                }
            }
        } else if (ts.token()->is_punct_of(PunctTokenKind::Dot)) {
            ast::Dot dot(ts.token()->span());
            ts.advance();

            TRY(check_ident(ctx, ts));
            std::string value = ts.token()->ident_value();
            ast::AccessExpressionField field(std::move(value),
                                             ts.token()->span());
            ts.advance();

            expr = std::make_unique<ast::AccessExpression>(
                std::move(*expr), dot, std::move(field));
        } else {
            break;
        }
    }
    return expr;
}

std::optional<std::unique_ptr<ast::Expression>> parse_primary_expr(
    Context &ctx, TokenStream &ts) {
    TRY(check_eos(ctx, ts));
    if (ts.token()->is_ident()) {
        std::string value1 = ts.token()->ident_value();
        auto span1 = ts.token()->span();
        ts.advance();

        if (!ts.is_eos() &&
            ts.token()->is_punct_of(PunctTokenKind::ColonColon)) {
            ast::ColonColon colon_colon(ts.token()->span());
            ts.advance();

            TRY(check_ident(ctx, ts));
            std::string value2 = ts.token()->ident_value();
            auto span2 = ts.token()->span();
            ts.advance();

            ast::EnumSelectExpressionSrc src(std::move(value1), span1);
            ast::EnumSelectExpressionDst dst(std::move(value1), span1);
            return std::make_unique<ast::EnumSelectExpression>(
                std::move(dst), colon_colon, std::move(src));
        } else if (!ts.is_eos() &&
                   ts.token()->is_punct_of(PunctTokenKind::LCurly)) {
            ast::LCurly lcurly(ts.token()->span());
            ts.advance();

            std::vector<ast::StructExpressionInit> inits;
            while (true) {
                TRY(check_eos(ctx, ts));
                if (ts.token()->is_punct_of(PunctTokenKind::RCurly)) {
                    break;
                } else if (ts.token()->is_ident()) {
                    ast::StructExpressionInitName name(
                        std::string(ts.token()->ident_value()),
                        ts.token()->span());
                    ts.advance();

                    TRY(check_punct(ctx, ts, PunctTokenKind::Colon));
                    ast::Colon colon(ts.token()->span());
                    ts.advance();

                    auto value = parse_expr(ctx, ts);
                    if (!value) return std::nullopt;

                    inits.emplace_back(std::move(name), colon,
                                       std::move(*value));

                    if (!ts.is_eos() &&
                        ts.token()->is_punct_of(PunctTokenKind::Comma)) {
                        ts.advance();
                    } else {
                        break;
                    }
                } else {
                    ReportInfo info(ts.token()->span(),
                                    "expected identifier or }", "");
                    report(ctx, ReportLevel::Error, info);
                    return std::nullopt;
                }
            }

            TRY(check_punct(ctx, ts, PunctTokenKind::RCurly));
            ast::RCurly rcurly(ts.token()->span());
            ts.advance();

            ast::StructExpressionName name(std::move(value1), span1);
            return std::make_unique<ast::StructExpression>(
                std::move(name), lcurly, std::move(inits), rcurly);
        } else {
            return std::make_unique<ast::VariableExpression>(std::move(value1),
                                                             span1);
        }
    } else if (ts.token()->is_punct_of(PunctTokenKind::LCurly)) {
        ast::LCurly lcurly(ts.token()->span());
        ts.advance();

        std::vector<std::unique_ptr<ast::Expression>> inits;
        while (true) {
            TRY(check_eos(ctx, ts));
            if (ts.token()->is_punct_of(PunctTokenKind::RCurly)) {
                break;
            } else {
                auto value = parse_expr(ctx, ts);
                if (!value) return std::nullopt;

                inits.emplace_back(std::move(*value));

                if (!ts.is_eos() &&
                    ts.token()->is_punct_of(PunctTokenKind::Comma)) {
                    ts.advance();
                } else {
                    break;
                }
            }
        }

        TRY(check_punct(ctx, ts, PunctTokenKind::RCurly));
        ast::RCurly rcurly(ts.token()->span());
        ts.advance();

        return std::make_unique<ast::ArrayExpression>(lcurly, std::move(inits),
                                                      rcurly);
    } else if (ts.token()->is_int()) {
        uint64_t value = ts.token()->int_value();
        auto span = ts.token()->span();
        ts.advance();

        return std::make_unique<ast::IntegerExpression>(value, span);
    } else if (ts.token()->is_string()) {
        std::string value = ts.token()->string_value();
        auto span = ts.token()->span();
        ts.advance();

        return std::make_unique<ast::StringExpression>(std::move(value), span);
    } else if (ts.token()->is_char()) {
        char value = ts.token()->char_value();
        auto span = ts.token()->span();
        ts.advance();

        return std::make_unique<ast::CharExpression>(value, span);
    } else if (ts.token()->is_punct_of(PunctTokenKind::LParen)) {
        ts.advance();
        auto expr = parse_expr(ctx, ts);
        if (!expr) return std::nullopt;

        TRY(check_punct(ctx, ts, PunctTokenKind::RParen));
        ts.advance();

        return expr;
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::True)) {
        auto span = ts.token()->span();
        ts.advance();

        return std::make_unique<ast::BoolExpression>(true, span);
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::False)) {
        auto span = ts.token()->span();
        ts.advance();

        return std::make_unique<ast::BoolExpression>(false, span);
    } else {
        ReportInfo info(ts.token()->span(), "unexpected token found",
                        "expected identifier, integer or `(`");
        report(ctx, ReportLevel::Error, info);
        return std::nullopt;
    }
}

std::optional<std::unique_ptr<ast::Type>> parse_type(Context &ctx,
                                                     TokenStream &ts) {
    TRY(check_eos(ctx, ts));
    if (ts.token()->is_keyword_of(KeywordTokenKind::ISize)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::ISize,
                                                       ts.token()->span());
        ts.advance();
        return type;
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::Int8)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::Int8,
                                                       ts.token()->span());
        ts.advance();
        return type;
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::Int16)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::Int16,
                                                       ts.token()->span());
        ts.advance();
        return type;
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::Int32)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::Int32,
                                                       ts.token()->span());
        ts.advance();
        return type;
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::Int64)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::Int64,
                                                       ts.token()->span());
        ts.advance();
        return type;
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::USize)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::USize,
                                                       ts.token()->span());
        ts.advance();
        return type;
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::UInt8)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::UInt8,
                                                       ts.token()->span());
        ts.advance();
        return type;
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::UInt16)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::UInt16,
                                                       ts.token()->span());
        ts.advance();
        return type;
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::UInt32)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::UInt32,
                                                       ts.token()->span());
        ts.advance();
        return type;
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::UInt64)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::UInt64,
                                                       ts.token()->span());
        ts.advance();
        return type;
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::Bool)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::Bool,
                                                       ts.token()->span());
        ts.advance();
        return type;
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::Char)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::Char,
                                                       ts.token()->span());
        ts.advance();
        return type;
    } else if (ts.token()->is_punct_of(PunctTokenKind::Star)) {
        ast::Star star(ts.token()->span());
        ts.advance();
        auto of = parse_type(ctx, ts);
        if (!of) return std::nullopt;
        return std::make_unique<ast::PointerType>(star, std::move(*of));
    } else if (ts.token()->is_punct_of(PunctTokenKind::LParen)) {
        return parse_array_type(ctx, ts);
    } else if (ts.token()->is_ident()) {
        auto name = ts.token()->ident_value();
        auto span = ts.token()->span();
        ts.advance();
        return std::make_unique<ast::NameType>(std::move(name), span);
    } else {
        ReportInfo info(ts.token()->span(),
                        "expected one of `int`, `uint`, `(` or identifier", "");
        report(ctx, ReportLevel::Error, info);
        ts.advance();
        return std::nullopt;
    }
}

std::optional<std::unique_ptr<ast::ArrayType>> parse_array_type(
    Context &ctx, TokenStream &ts) {
    TRY(check_punct(ctx, ts, PunctTokenKind::LParen));
    ast::LParen lparen(ts.token()->span());
    ts.advance();

    auto of = parse_type(ctx, ts);
    if (!of) return std::nullopt;

    TRY(check_punct(ctx, ts, PunctTokenKind::RParen));
    ast::RParen rparen(ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LSquare));
    ast::LSquare lsquare(ts.token()->span());
    ts.advance();

    TRY(check_eos(ctx, ts));
    std::optional<std::unique_ptr<ast::Expression>> size;
    if (!ts.token()->is_punct_of(PunctTokenKind::RSquare)) {
        size = parse_expr(ctx, ts);
        if (!size) return std::nullopt;
    }

    TRY(check_punct(ctx, ts, PunctTokenKind::RSquare));
    ast::RSquare rsquare(ts.token()->span());
    ts.advance();

    return std::make_unique<ast::ArrayType>(lparen, std::move(*of), rparen,
                                            lsquare, std::move(*size), rsquare);
}

std::optional<std::unique_ptr<ast::Statement>> parse_stmt(Context &ctx,
                                                          TokenStream &ts) {
    TRY(check_eos(ctx, ts));
    if (ts.token()->is_keyword_of(KeywordTokenKind::Return)) {
        return parse_return_stmt(ctx, ts);
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::Break)) {
        return parse_break_stmt(ctx, ts);
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::Continue)) {
        return parse_continue_stmt(ctx, ts);
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::While)) {
        return parse_while_stmt(ctx, ts);
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::If)) {
        return parse_if_stmt(ctx, ts);
    } else if (ts.token()->is_punct_of(PunctTokenKind::LCurly)) {
        return parse_block_stmt(ctx, ts);
    } else {
        return parse_expr_stmt(ctx, ts);
    }
}

std::optional<std::unique_ptr<ast::ExpressionStatement>> parse_expr_stmt(
    Context &ctx, TokenStream &ts) {
    auto expr = parse_expr(ctx, ts);
    if (!expr) return std::nullopt;

    TRY(check_punct(ctx, ts, PunctTokenKind::Semicolon));
    ast::Semicolon semicolon(ts.token()->span());
    ts.advance();

    return std::make_unique<ast::ExpressionStatement>(std::move(*expr),
                                                      semicolon);
}

std::optional<std::unique_ptr<ast::ReturnStatement>> parse_return_stmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Return));
    ast::Return return_kw(ts.token()->span());
    ts.advance();

    std::optional<std::unique_ptr<ast::Expression>> expr;
    TRY(check_eos(ctx, ts));
    if (!ts.token()->is_punct_of(PunctTokenKind::Semicolon)) {
        expr = parse_expr(ctx, ts);
        if (!expr) return std::nullopt;
    }

    TRY(check_punct(ctx, ts, PunctTokenKind::Semicolon));
    ast::Semicolon semicolon(ts.token()->span());
    ts.advance();

    return std::make_unique<ast::ReturnStatement>(return_kw, std::move(*expr),
                                                  semicolon);
}

std::optional<std::unique_ptr<ast::BreakStatement>> parse_break_stmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Break));
    ast::Break break_kw(ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::Semicolon));
    ast::Semicolon semicolon(ts.token()->span());
    ts.advance();

    return std::make_unique<ast::BreakStatement>(break_kw, semicolon);
}

std::optional<std::unique_ptr<ast::ContinueStatement>> parse_continue_stmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Continue));
    ast::Continue continue_kw(ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::Semicolon));
    ast::Semicolon semicolon(ts.token()->span());
    ts.advance();

    return std::make_unique<ast::ContinueStatement>(continue_kw, semicolon);
}

std::optional<std::unique_ptr<ast::WhileStatement>> parse_while_stmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::While));
    ast::While while_kw(ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LParen));
    ast::LParen lparen(ts.token()->span());
    ts.advance();

    auto cond = parse_expr(ctx, ts);
    if (!cond) return std::nullopt;

    TRY(check_punct(ctx, ts, PunctTokenKind::RParen));
    ast::RParen rparen(ts.token()->span());
    ts.advance();

    auto body = parse_stmt(ctx, ts);
    if (!body) return std::nullopt;

    return std::make_unique<ast::WhileStatement>(
        while_kw, lparen, std::move(*cond), rparen, std::move(*body));
}

std::optional<std::unique_ptr<ast::IfStatement>> parse_if_stmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::If));
    ast::If if_kw(ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LParen));
    ast::LParen lparen(ts.token()->span());
    ts.advance();

    auto cond = parse_expr(ctx, ts);
    if (!cond) return std::nullopt;

    TRY(check_punct(ctx, ts, PunctTokenKind::RParen));
    ast::RParen rparen(ts.token()->span());
    ts.advance();

    auto body = parse_stmt(ctx, ts);
    if (!body) return std::nullopt;

    std::optional<ast::IfStatementElseClause> else_clause;
    if (!ts.is_eos() && ts.token()->is_keyword_of(KeywordTokenKind::Else)) {
        ast::Else else_kw(ts.token()->span());
        ts.advance();

        auto else_body = parse_stmt(ctx, ts);
        if (!else_body) return std::nullopt;

        else_clause.emplace(else_kw, std::move(*else_body));
    }

    return std::make_unique<ast::IfStatement>(if_kw, lparen, std::move(*cond),
                                              rparen, std::move(*body),
                                              std::move(else_clause));
}

std::optional<std::unique_ptr<ast::BlockStatement>> parse_block_stmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_punct(ctx, ts, PunctTokenKind::LCurly));
    ast::LCurly lcurly(ts.token()->span());
    ts.advance();

    std::vector<ast::VariableDeclarations> decls;
    while (true) {
        TRY(check_eos(ctx, ts));
        if (ts.token()->is_keyword_of(KeywordTokenKind::Let)) {
            ast::Let let_kw(ts.token()->span());
            ts.advance();

            std::vector<ast::VariableDeclarationBody> names;
            while (true) {
                TRY(check_ident(ctx, ts));
                std::string value = ts.token()->ident_value();
                ast::VariableName name(std::move(value), ts.token()->span());
                ts.advance();

                TRY(check_punct(ctx, ts, PunctTokenKind::Colon));
                ast::Colon colon(ts.token()->span());
                ts.advance();

                auto type = parse_type(ctx, ts);
                if (!type) return std::nullopt;

                std::optional<ast::VariableInit> init;
                if (!ts.is_eos() &&
                    ts.token()->is_punct_of(PunctTokenKind::Assign)) {
                    ast::Assign assign(ts.token()->span());
                    ts.advance();

                    auto expr = parse_expr(ctx, ts);
                    if (!expr) return std::nullopt;

                    init.emplace(assign, std::move(*expr));
                }

                names.emplace_back(std::move(name), colon, std::move(*type),
                                   std::move(init));

                TRY(check_eos(ctx, ts));
                if (ts.token()->is_punct_of(PunctTokenKind::Comma)) {
                    ts.advance();
                } else {
                    break;
                }
            }

            TRY(check_punct(ctx, ts, PunctTokenKind::Semicolon));
            ast::Semicolon semicolon(ts.token()->span());
            ts.advance();

            decls.emplace_back(let_kw, std::move(names), semicolon);
        } else {
            break;
        }
    }

    std::vector<std::unique_ptr<ast::Statement>> stmts;
    while (true) {
        TRY(check_eos(ctx, ts));
        if (ts.token()->is_punct_of(PunctTokenKind::RCurly)) {
            ast::RCurly rcurly(ts.token()->span());
            ts.advance();
            return std::make_unique<ast::BlockStatement>(
                lcurly, std::move(decls), std::move(stmts), rcurly);
        } else {
            auto stmt = parse_stmt(ctx, ts);
            if (!stmt) return std::nullopt;
            stmts.emplace_back(std::move(*stmt));
        }
    }
}

std::optional<std::unique_ptr<ast::Declaration>> parse_decl(Context &ctx,
                                                            TokenStream &ts) {
    if (ts.token()->is_keyword_of(KeywordTokenKind::Function)) {
        return parse_func_decl(ctx, ts);
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::Struct)) {
        return parse_struct_decl(ctx, ts);
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::Enum)) {
        return parse_enum_decl(ctx, ts);
    } else {
        ReportInfo info(ts.token()->span(),
                        "expected one of `function`, `struct` or `enum`", "");
        report(ctx, ReportLevel::Error, info);
        return std::nullopt;
    }
}

std::optional<std::unique_ptr<ast::FunctionDeclaration>> parse_func_decl(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Function));
    ast::Function function_kw(ts.token()->span());
    ts.advance();

    TRY(check_ident(ctx, ts));
    std::string value = ts.token()->ident_value();
    ast::FunctionDeclarationName name(std::move(value), ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LParen));
    ast::LParen lparen(ts.token()->span());
    ts.advance();

    TRY(check_eos(ctx, ts));
    std::vector<ast::FunctionDeclarationParameter> params;
    if (!ts.token()->is_punct_of(PunctTokenKind::RParen)) {
        while (true) {
            TRY(check_eos(ctx, ts));
            if (ts.token()->is_ident()) {
                std::string value = ts.token()->ident_value();
                ast::FunctionDeclarationParameterName name(std::move(value),
                                                           ts.token()->span());
                ts.advance();

                TRY(check_punct(ctx, ts, PunctTokenKind::Colon));
                ast::Colon colon(ts.token()->span());
                ts.advance();

                auto type = parse_type(ctx, ts);
                if (!type) return std::nullopt;

                params.emplace_back(std::move(name), colon, std::move(*type));

                TRY(check_eos(ctx, ts));
                if (ts.token()->is_punct_of(PunctTokenKind::RParen)) {
                    break;
                } else if (ts.token()->is_punct_of(PunctTokenKind::Comma)) {
                    ts.advance();
                }
            } else if (ts.token()->is_punct_of(PunctTokenKind::RParen) &&
                       params.empty()) {
                break;
            } else {
                ReportInfo info(ts.token()->span(), "unexpected token found",
                                "expected identifier or `)`");
                report(ctx, ReportLevel::Error, info);
                return std::nullopt;
            }
        }
    }

    TRY(check_punct(ctx, ts, PunctTokenKind::RParen));
    ast::RParen rparen(ts.token()->span());
    ts.advance();

    std::optional<ast::FunctionDeclarationReturn> ret;
    if (!ts.is_eos() && ts.token()->is_punct_of(PunctTokenKind::Arrow)) {
        ast::Arrow arrow(ts.token()->span());
        ts.advance();

        auto type = parse_type(ctx, ts);
        if (!type) return std::nullopt;

        ret.emplace(arrow, std::move(*type));
    }

    auto body = parse_block_stmt(ctx, ts);
    if (!body) return std::nullopt;

    return std::make_unique<ast::FunctionDeclaration>(
        function_kw, std::move(name), lparen, std::move(params), rparen,
        std::move(ret), std::move(*body));
}

std::optional<std::unique_ptr<ast::StructDeclaration>> parse_struct_decl(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Struct));
    ast::Struct struct_kw(ts.token()->span());
    ts.advance();

    TRY(check_ident(ctx, ts));
    std::string value = ts.token()->ident_value();
    ast::StructDeclarationName name(std::move(value), ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LCurly));
    ast::LCurly lcurly(ts.token()->span());
    ts.advance();

    std::vector<ast::StructDeclarationField> fields;
    while (true) {
        TRY(check_eos(ctx, ts));
        if (ts.token()->is_ident()) {
            std::string value = ts.token()->ident_value();
            ast::StructDeclarationFieldName name(std::move(value),
                                                 ts.token()->span());
            ts.advance();

            TRY(check_punct(ctx, ts, PunctTokenKind::Colon));
            ast::Colon colon(ts.token()->span());
            ts.advance();

            auto type = parse_type(ctx, ts);
            if (!type) return std::nullopt;

            fields.emplace_back(std::move(name), colon, std::move(*type));

            if (!ts.is_eos() &&
                ts.token()->is_punct_of(PunctTokenKind::Comma)) {
                ts.advance();
            } else {
                break;
            }
        } else if (ts.token()->is_punct_of(PunctTokenKind::RCurly)) {
            break;
        } else {
            if (ts.has_prev()) {
                ReportInfo info(ts.prev()->span(), "unexpected token",
                                "expected identifier or `}` after this");
                report(ctx, ReportLevel::Error, info);
                return std::nullopt;
            } else {
                ReportInfo info(ts.token()->span(), "unexpected token",
                                "expected this to be identifier or `}`");
                report(ctx, ReportLevel::Error, info);
                return std::nullopt;
            }
        }
    }

    TRY(check_punct(ctx, ts, PunctTokenKind::RCurly));
    ast::RCurly rcurly(ts.token()->span());
    ts.advance();

    return std::make_unique<ast::StructDeclaration>(
        struct_kw, std::move(name), lcurly, std::move(fields), rcurly);
}

std::optional<std::unique_ptr<ast::EnumDeclaration>> parse_enum_decl(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Enum));
    ast::Enum enum_kw(ts.token()->span());
    ts.advance();

    TRY(check_ident(ctx, ts));
    std::string value = ts.token()->ident_value();
    ast::EnumDeclarationName name(std::move(value), ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LCurly));
    ast::LCurly lcurly(ts.token()->span());
    ts.advance();

    std::vector<ast::EnumDeclarationField> fields;
    while (true) {
        TRY(check_eos(ctx, ts));
        if (ts.token()->is_ident()) {
            std::string name = ts.token()->ident_value();
            fields.emplace_back(std::move(name), ts.token()->span());
            ts.advance();

            if (!ts.is_eos() &&
                ts.token()->is_punct_of(PunctTokenKind::Comma)) {
                ts.advance();
            } else {
                break;
            }
        } else if (ts.token()->is_punct_of(PunctTokenKind::RCurly)) {
            break;
        } else {
            if (ts.has_prev()) {
                ReportInfo info(ts.prev()->span(), "unexpected token",
                                "expected identifier or `}` after this");
                report(ctx, ReportLevel::Error, info);
                return std::nullopt;
            } else {
                ReportInfo info(ts.token()->span(), "unexpected token",
                                "expected this to be identifier or `}`");
                report(ctx, ReportLevel::Error, info);
                return std::nullopt;
            }
        }
    }

    TRY(check_punct(ctx, ts, PunctTokenKind::RCurly));
    ast::RCurly rcurly(ts.token()->span());
    ts.advance();

    return std::make_unique<ast::EnumDeclaration>(
        enum_kw, std::move(name), lcurly, std::move(fields), rcurly);
}

ParserResult parse_file(Context &ctx, const std::string &path) {
    auto tokens = lex_file(ctx, path);
    if (!tokens) return std::nullopt;
    TokenStream ts(std::move(*tokens));

    std::vector<std::unique_ptr<ast::Declaration>> res;
    while (!ts.is_eos()) {
        auto decl = parse_decl(ctx, ts);
        if (!decl.has_value()) return std::nullopt;
        res.emplace_back(std::move(*decl));
    }
    return res;
}

};  // namespace mini
