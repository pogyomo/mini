#include "parser.h"

#include <memory>
#include <optional>
#include <vector>

#include "ast/decl.h"
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
    if (!ts) {
        ReportInfo info(ts.Last()->span(), "expected token after this", "");
        Report(ctx, ReportLevel::Error, info);
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
        if (!ts.CurrToken()->IsIdent()) {
            if (ts.HasPrev()) {
                ReportInfo info(ts.PrevToken()->span(),
                                "expected identifier after this", "");
                Report(ctx, ReportLevel::Error, info);
                return true;
            } else {
                ReportInfo info(ts.CurrToken()->span(),
                                "expected this to be identifier", "");
                Report(ctx, ReportLevel::Error, info);
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
        if (!ts.CurrToken()->IsPunctOf(kind)) {
            if (ts.HasPrev()) {
                ReportInfo info(ts.PrevToken()->span(),
                                "expected `" + ToString(kind) + "` after this",
                                "");
                Report(ctx, ReportLevel::Error, info);
                return true;
            } else {
                ReportInfo info(ts.CurrToken()->span(),
                                "expected this to be `" + ToString(kind) + "`",
                                "");
                Report(ctx, ReportLevel::Error, info);
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
        if (!ts.CurrToken()->IsKeywordOf(kind)) {
            if (ts.HasPrev()) {
                ReportInfo info(ts.PrevToken()->span(),
                                "expected `" + ToString(kind) + "` after this",
                                "");
                Report(ctx, ReportLevel::Error, info);
                return true;
            } else {
                ReportInfo info(ts.CurrToken()->span(),
                                "expected this to be `" + ToString(kind) + "`",
                                "");
                Report(ctx, ReportLevel::Error, info);
                return true;
            }
        } else {
            return false;
        }
    }
}

std::optional<std::unique_ptr<ast::Expression>> ParseExpr(Context &ctx,
                                                          TokenStream &ts) {
    auto state = ts.State();
    ctx.SuppressReport();

    auto lhs = ParseUnaryExpr(ctx, ts);
    if (!lhs) {
        ts.SetState(state);
        ctx.ActivateReport();
        return ParseLogicalOrExpr(ctx, ts);
    }

    if (!ts || !ts.CurrToken()->IsPunctOf(PunctTokenKind::Assign)) {
        ts.SetState(state);
        ctx.ActivateReport();
        return ParseLogicalOrExpr(ctx, ts);
    }
    ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::Assign,
                                ts.CurrToken()->span());
    ts.Advance();

    ctx.ActivateReport();
    auto rhs = ParseExpr(ctx, ts);
    if (!rhs) return std::nullopt;

    return std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                  std::move(*rhs));
}

std::optional<std::unique_ptr<ast::Expression>> ParseLogicalOrExpr(
    Context &ctx, TokenStream &ts) {
    auto lhs = ParseLogicalAndExpr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (ts) {
        if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Or)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::Or,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseLogicalOrExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> ParseLogicalAndExpr(
    Context &ctx, TokenStream &ts) {
    auto lhs = ParseInclusiveOrExpr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (ts) {
        if (ts.CurrToken()->IsPunctOf(PunctTokenKind::And)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::And,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseLogicalAndExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> ParseInclusiveOrExpr(
    Context &ctx, TokenStream &ts) {
    auto lhs = ParseExclusiveOrExpr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (ts) {
        if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Vertical)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::BitOr,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseInclusiveOrExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> ParseExclusiveOrExpr(
    Context &ctx, TokenStream &ts) {
    auto lhs = ParseAndExpr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (ts) {
        if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Hat)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::BitXor,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseExclusiveOrExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> ParseAndExpr(Context &ctx,
                                                             TokenStream &ts) {
    auto lhs = ParseEqualityExpr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (ts) {
        if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Ampersand)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::BitAnd,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseAndExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> ParseEqualityExpr(
    Context &ctx, TokenStream &ts) {
    auto lhs = ParseRelationalExpr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (ts) {
        if (ts.CurrToken()->IsPunctOf(PunctTokenKind::EQ)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::EQ,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseEqualityExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::NE)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::NE,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseEqualityExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> ParseRelationalExpr(
    Context &ctx, TokenStream &ts) {
    auto lhs = ParseShiftExpr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (ts) {
        if (ts.CurrToken()->IsPunctOf(PunctTokenKind::LT)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::LT,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseRelationalExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::LE)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::LE,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseRelationalExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::GT)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::GT,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseRelationalExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::GE)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::GE,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseRelationalExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> ParseShiftExpr(
    Context &ctx, TokenStream &ts) {
    auto lhs = ParseAdditiveExpr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (ts) {
        if (ts.CurrToken()->IsPunctOf(PunctTokenKind::LShift)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::LShift,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseShiftExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::RShift)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::RShift,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseShiftExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> ParseAdditiveExpr(
    Context &ctx, TokenStream &ts) {
    auto lhs = ParseMultiplicativeExpr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (ts) {
        if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Plus)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::Add,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseAdditiveExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Minus)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::Sub,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseAdditiveExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> ParseMultiplicativeExpr(
    Context &ctx, TokenStream &ts) {
    auto lhs = ParseCastExpr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (ts) {
        if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Star)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::Mul,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseMultiplicativeExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Slash)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::Div,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseMultiplicativeExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Percent)) {
            ast::InfixExpression::Op op(ast::InfixExpression::Op::Kind::Mod,
                                        ts.CurrToken()->span());
            ts.Advance();
            auto rhs = ParseMultiplicativeExpr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<ast::InfixExpression>(op, std::move(*lhs),
                                                         std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<ast::Expression>> ParseCastExpr(Context &ctx,
                                                              TokenStream &ts) {
    auto expr = ParseUnaryExpr(ctx, ts);
    if (!expr) return std::nullopt;
    while (ts) {
        if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::As)) {
            ast::As as_kw(ts.CurrToken()->span());
            ts.Advance();

            auto type = ParseType(ctx, ts);
            if (!type) return std::nullopt;

            expr = std::make_unique<ast::CastExpression>(
                std::move(*expr), as_kw, std::move(*type));
        } else {
            break;
        }
    }
    return expr;
}

std::optional<std::unique_ptr<ast::Expression>> ParseUnaryExpr(
    Context &ctx, TokenStream &ts) {
    TRY(check_eos(ctx, ts));
    if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Ampersand)) {
        ast::UnaryExpression::Op op(ast::UnaryExpression::Op::Kind::Ref,
                                    ts.CurrToken()->span());
        ts.Advance();

        auto expr = ParseUnaryExpr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<ast::UnaryExpression>(op, std::move(*expr));
    } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Star)) {
        ast::UnaryExpression::Op op(ast::UnaryExpression::Op::Kind::Deref,
                                    ts.CurrToken()->span());
        ts.Advance();

        auto expr = ParseUnaryExpr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<ast::UnaryExpression>(op, std::move(*expr));
    } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Minus)) {
        ast::UnaryExpression::Op op(ast::UnaryExpression::Op::Kind::Minus,
                                    ts.CurrToken()->span());
        ts.Advance();

        auto expr = ParseUnaryExpr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<ast::UnaryExpression>(op, std::move(*expr));
    } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Tilde)) {
        ast::UnaryExpression::Op op(ast::UnaryExpression::Op::Kind::Inv,
                                    ts.CurrToken()->span());
        ts.Advance();

        auto expr = ParseUnaryExpr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<ast::UnaryExpression>(op, std::move(*expr));
    } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Exclamation)) {
        ast::UnaryExpression::Op op(ast::UnaryExpression::Op::Kind::Neg,
                                    ts.CurrToken()->span());
        ts.Advance();

        auto expr = ParseUnaryExpr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<ast::UnaryExpression>(op, std::move(*expr));
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::ESizeof)) {
        ast::ESizeof esizeof_kw(ts.CurrToken()->span());
        ts.Advance();

        auto expr = ParseExpr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<ast::ESizeofExpression>(esizeof_kw,
                                                        std::move(*expr));
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::TSizeof)) {
        ast::TSizeof tsizeof_kw(ts.CurrToken()->span());
        ts.Advance();

        auto type = ParseType(ctx, ts);
        if (!type) return std::nullopt;

        return std::make_unique<ast::TSizeofExpression>(tsizeof_kw,
                                                        std::move(*type));
    } else {
        return ParsePostfixExpr(ctx, ts);
    }
}

std::optional<std::unique_ptr<ast::Expression>> ParsePostfixExpr(
    Context &ctx, TokenStream &ts) {
    auto expr = ParsePrimaryExpr(ctx, ts);
    if (!expr) return std::nullopt;
    while (ts) {
        if (ts.CurrToken()->IsPunctOf(PunctTokenKind::LSquare)) {
            ast::LSquare lsquare(ts.CurrToken()->span());
            ts.Advance();

            auto index = ParseExpr(ctx, ts);
            if (!index) return std::nullopt;

            TRY(check_punct(ctx, ts, PunctTokenKind::RSquare));
            ast::RSquare rsquare(ts.CurrToken()->span());
            ts.Advance();

            expr = std::make_unique<ast::IndexExpression>(
                std::move(*expr), lsquare, std::move(*index), rsquare);
        } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::LParen)) {
            ast::LParen lparen(ts.CurrToken()->span());
            ts.Advance();

            std::vector<std::unique_ptr<ast::Expression>> args;
            while (true) {
                TRY(check_eos(ctx, ts));
                if (ts.CurrToken()->IsPunctOf(PunctTokenKind::RParen) &&
                    args.empty()) {
                    ast::RParen rparen(ts.CurrToken()->span());
                    ts.Advance();

                    expr = std::make_unique<ast::CallExpression>(
                        std::move(*expr), lparen, std::move(args), rparen);
                    break;
                } else {
                    auto arg = ParseExpr(ctx, ts);
                    if (!arg) return std::nullopt;

                    args.emplace_back(std::move(*arg));

                    if (ts.CurrToken()->IsPunctOf(PunctTokenKind::RParen)) {
                        ast::RParen rparen(ts.CurrToken()->span());
                        ts.Advance();

                        expr = std::make_unique<ast::CallExpression>(
                            std::move(*expr), lparen, std::move(args), rparen);
                        break;
                    } else if (ts.CurrToken()->IsPunctOf(
                                   PunctTokenKind::Comma)) {
                        ts.Advance();
                        continue;
                    } else {
                        ReportInfo info(ts.CurrToken()->span(),
                                        "unexpected token",
                                        "expected `)` or `,`");
                        Report(ctx, ReportLevel::Error, info);
                        return std::nullopt;
                    }
                }
            }
        } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Dot)) {
            ast::Dot dot(ts.CurrToken()->span());
            ts.Advance();

            TRY(check_ident(ctx, ts));
            std::string value = ts.CurrToken()->IdentValue();
            ast::AccessExpressionField field(std::move(value),
                                             ts.CurrToken()->span());
            ts.Advance();

            expr = std::make_unique<ast::AccessExpression>(
                std::move(*expr), dot, std::move(field));
        } else {
            break;
        }
    }
    return expr;
}

std::optional<std::unique_ptr<ast::Expression>> ParsePrimaryExpr(
    Context &ctx, TokenStream &ts) {
    TRY(check_eos(ctx, ts));
    if (ts.CurrToken()->IsIdent()) {
        std::string value1 = ts.CurrToken()->IdentValue();
        auto span1 = ts.CurrToken()->span();
        ts.Advance();

        if (ts && ts.CurrToken()->IsPunctOf(PunctTokenKind::ColonColon)) {
            ast::ColonColon colon_colon(ts.CurrToken()->span());
            ts.Advance();

            TRY(check_ident(ctx, ts));
            std::string value2 = ts.CurrToken()->IdentValue();
            auto span2 = ts.CurrToken()->span();
            ts.Advance();

            ast::EnumSelectExpressionSrc src(std::move(value1), span1);
            ast::EnumSelectExpressionDst dst(std::move(value1), span1);
            return std::make_unique<ast::EnumSelectExpression>(
                std::move(dst), colon_colon, std::move(src));
        } else if (ts && ts.CurrToken()->IsPunctOf(PunctTokenKind::LCurly)) {
            ast::LCurly lcurly(ts.CurrToken()->span());
            ts.Advance();

            std::vector<ast::StructExpressionInit> inits;
            while (true) {
                TRY(check_eos(ctx, ts));
                if (ts.CurrToken()->IsPunctOf(PunctTokenKind::RCurly)) {
                    break;
                } else if (ts.CurrToken()->IsIdent()) {
                    ast::StructExpressionInitName name(
                        std::string(ts.CurrToken()->IdentValue()),
                        ts.CurrToken()->span());
                    ts.Advance();

                    TRY(check_punct(ctx, ts, PunctTokenKind::Colon));
                    ast::Colon colon(ts.CurrToken()->span());
                    ts.Advance();

                    auto value = ParseExpr(ctx, ts);
                    if (!value) return std::nullopt;

                    inits.emplace_back(std::move(name), colon,
                                       std::move(*value));

                    if (ts &&
                        ts.CurrToken()->IsPunctOf(PunctTokenKind::Comma)) {
                        ts.Advance();
                    } else {
                        break;
                    }
                } else {
                    ReportInfo info(ts.CurrToken()->span(),
                                    "expected identifier or }", "");
                    Report(ctx, ReportLevel::Error, info);
                    return std::nullopt;
                }
            }

            TRY(check_punct(ctx, ts, PunctTokenKind::RCurly));
            ast::RCurly rcurly(ts.CurrToken()->span());
            ts.Advance();

            ast::StructExpressionName name(std::move(value1), span1);
            return std::make_unique<ast::StructExpression>(
                std::move(name), lcurly, std::move(inits), rcurly);
        } else {
            return std::make_unique<ast::VariableExpression>(std::move(value1),
                                                             span1);
        }
    } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::LCurly)) {
        ast::LCurly lcurly(ts.CurrToken()->span());
        ts.Advance();

        std::vector<std::unique_ptr<ast::Expression>> inits;
        while (true) {
            TRY(check_eos(ctx, ts));
            if (ts.CurrToken()->IsPunctOf(PunctTokenKind::RCurly)) {
                break;
            } else {
                auto value = ParseExpr(ctx, ts);
                if (!value) return std::nullopt;

                inits.emplace_back(std::move(*value));

                if (ts && ts.CurrToken()->IsPunctOf(PunctTokenKind::Comma)) {
                    ts.Advance();
                } else {
                    break;
                }
            }
        }

        TRY(check_punct(ctx, ts, PunctTokenKind::RCurly));
        ast::RCurly rcurly(ts.CurrToken()->span());
        ts.Advance();

        return std::make_unique<ast::ArrayExpression>(lcurly, std::move(inits),
                                                      rcurly);
    } else if (ts.CurrToken()->IsInt()) {
        uint64_t value = ts.CurrToken()->IntValue();
        auto span = ts.CurrToken()->span();
        ts.Advance();

        return std::make_unique<ast::IntegerExpression>(value, span);
    } else if (ts.CurrToken()->IsString()) {
        std::string value = ts.CurrToken()->StringValue();
        auto span = ts.CurrToken()->span();
        ts.Advance();

        return std::make_unique<ast::StringExpression>(std::move(value), span);
    } else if (ts.CurrToken()->IsChar()) {
        char value = ts.CurrToken()->CharValue();
        auto span = ts.CurrToken()->span();
        ts.Advance();

        return std::make_unique<ast::CharExpression>(value, span);
    } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::LParen)) {
        ts.Advance();
        auto expr = ParseExpr(ctx, ts);
        if (!expr) return std::nullopt;

        TRY(check_punct(ctx, ts, PunctTokenKind::RParen));
        ts.Advance();

        return expr;
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::True)) {
        auto span = ts.CurrToken()->span();
        ts.Advance();

        return std::make_unique<ast::BoolExpression>(true, span);
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::False)) {
        auto span = ts.CurrToken()->span();
        ts.Advance();

        return std::make_unique<ast::BoolExpression>(false, span);
    } else {
        ReportInfo info(ts.CurrToken()->span(), "unexpected token found",
                        "expected identifier, integer or `(`");
        Report(ctx, ReportLevel::Error, info);
        return std::nullopt;
    }
}

std::optional<std::unique_ptr<ast::Type>> ParseType(Context &ctx,
                                                    TokenStream &ts) {
    TRY(check_eos(ctx, ts));
    if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Void)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::Void,
                                                       ts.CurrToken()->span());
        ts.Advance();
        return type;
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::ISize)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::ISize,
                                                       ts.CurrToken()->span());
        ts.Advance();
        return type;
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Int8)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::Int8,
                                                       ts.CurrToken()->span());
        ts.Advance();
        return type;
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Int16)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::Int16,
                                                       ts.CurrToken()->span());
        ts.Advance();
        return type;
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Int32)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::Int32,
                                                       ts.CurrToken()->span());
        ts.Advance();
        return type;
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Int64)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::Int64,
                                                       ts.CurrToken()->span());
        ts.Advance();
        return type;
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::USize)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::USize,
                                                       ts.CurrToken()->span());
        ts.Advance();
        return type;
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::UInt8)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::UInt8,
                                                       ts.CurrToken()->span());
        ts.Advance();
        return type;
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::UInt16)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::UInt16,
                                                       ts.CurrToken()->span());
        ts.Advance();
        return type;
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::UInt32)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::UInt32,
                                                       ts.CurrToken()->span());
        ts.Advance();
        return type;
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::UInt64)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::UInt64,
                                                       ts.CurrToken()->span());
        ts.Advance();
        return type;
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Bool)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::Bool,
                                                       ts.CurrToken()->span());
        ts.Advance();
        return type;
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Char)) {
        auto type = std::make_unique<ast::BuiltinType>(ast::BuiltinType::Char,
                                                       ts.CurrToken()->span());
        ts.Advance();
        return type;
    } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Star)) {
        ast::Star star(ts.CurrToken()->span());
        ts.Advance();
        auto of = ParseType(ctx, ts);
        if (!of) return std::nullopt;
        return std::make_unique<ast::PointerType>(star, std::move(*of));
    } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::LParen)) {
        return ParseArrayType(ctx, ts);
    } else if (ts.CurrToken()->IsIdent()) {
        auto name = ts.CurrToken()->IdentValue();
        auto span = ts.CurrToken()->span();
        ts.Advance();
        return std::make_unique<ast::NameType>(std::move(name), span);
    } else {
        ReportInfo info(ts.CurrToken()->span(),
                        "expected one of `int`, `uint`, `(` or identifier", "");
        Report(ctx, ReportLevel::Error, info);
        ts.Advance();
        return std::nullopt;
    }
}

std::optional<std::unique_ptr<ast::ArrayType>> ParseArrayType(Context &ctx,
                                                              TokenStream &ts) {
    TRY(check_punct(ctx, ts, PunctTokenKind::LParen));
    ast::LParen lparen(ts.CurrToken()->span());
    ts.Advance();

    auto of = ParseType(ctx, ts);
    if (!of) return std::nullopt;

    TRY(check_punct(ctx, ts, PunctTokenKind::RParen));
    ast::RParen rparen(ts.CurrToken()->span());
    ts.Advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LSquare));
    ast::LSquare lsquare(ts.CurrToken()->span());
    ts.Advance();

    TRY(check_eos(ctx, ts));
    std::optional<std::unique_ptr<ast::Expression>> size;
    if (!ts.CurrToken()->IsPunctOf(PunctTokenKind::RSquare)) {
        size = ParseExpr(ctx, ts);
        if (!size) return std::nullopt;
    }

    TRY(check_punct(ctx, ts, PunctTokenKind::RSquare));
    ast::RSquare rsquare(ts.CurrToken()->span());
    ts.Advance();

    return std::make_unique<ast::ArrayType>(lparen, std::move(*of), rparen,
                                            lsquare, std::move(*size), rsquare);
}

std::optional<std::unique_ptr<ast::Statement>> ParseStmt(Context &ctx,
                                                         TokenStream &ts) {
    TRY(check_eos(ctx, ts));
    if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Return)) {
        return ParseReturnStmt(ctx, ts);
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Break)) {
        return ParseBreakStmt(ctx, ts);
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Continue)) {
        return ParseContinueStmt(ctx, ts);
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::While)) {
        return ParseWhileStmt(ctx, ts);
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::If)) {
        return ParseIfStmt(ctx, ts);
    } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::LCurly)) {
        return ParseBlockStmt(ctx, ts);
    } else {
        return ParseExprStmt(ctx, ts);
    }
}

std::optional<std::unique_ptr<ast::ExpressionStatement>> ParseExprStmt(
    Context &ctx, TokenStream &ts) {
    auto expr = ParseExpr(ctx, ts);
    if (!expr) return std::nullopt;

    TRY(check_punct(ctx, ts, PunctTokenKind::Semicolon));
    ast::Semicolon semicolon(ts.CurrToken()->span());
    ts.Advance();

    return std::make_unique<ast::ExpressionStatement>(std::move(*expr),
                                                      semicolon);
}

std::optional<std::unique_ptr<ast::ReturnStatement>> ParseReturnStmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Return));
    ast::Return return_kw(ts.CurrToken()->span());
    ts.Advance();

    std::optional<std::unique_ptr<ast::Expression>> expr;
    TRY(check_eos(ctx, ts));
    if (!ts.CurrToken()->IsPunctOf(PunctTokenKind::Semicolon)) {
        expr = ParseExpr(ctx, ts);
        if (!expr) return std::nullopt;
    }

    TRY(check_punct(ctx, ts, PunctTokenKind::Semicolon));
    ast::Semicolon semicolon(ts.CurrToken()->span());
    ts.Advance();

    return std::make_unique<ast::ReturnStatement>(return_kw, std::move(expr),
                                                  semicolon);
}

std::optional<std::unique_ptr<ast::BreakStatement>> ParseBreakStmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Break));
    ast::Break break_kw(ts.CurrToken()->span());
    ts.Advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::Semicolon));
    ast::Semicolon semicolon(ts.CurrToken()->span());
    ts.Advance();

    return std::make_unique<ast::BreakStatement>(break_kw, semicolon);
}

std::optional<std::unique_ptr<ast::ContinueStatement>> ParseContinueStmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Continue));
    ast::Continue continue_kw(ts.CurrToken()->span());
    ts.Advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::Semicolon));
    ast::Semicolon semicolon(ts.CurrToken()->span());
    ts.Advance();

    return std::make_unique<ast::ContinueStatement>(continue_kw, semicolon);
}

std::optional<std::unique_ptr<ast::WhileStatement>> ParseWhileStmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::While));
    ast::While while_kw(ts.CurrToken()->span());
    ts.Advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LParen));
    ast::LParen lparen(ts.CurrToken()->span());
    ts.Advance();

    auto cond = ParseExpr(ctx, ts);
    if (!cond) return std::nullopt;

    TRY(check_punct(ctx, ts, PunctTokenKind::RParen));
    ast::RParen rparen(ts.CurrToken()->span());
    ts.Advance();

    auto body = ParseStmt(ctx, ts);
    if (!body) return std::nullopt;

    return std::make_unique<ast::WhileStatement>(
        while_kw, lparen, std::move(*cond), rparen, std::move(*body));
}

std::optional<std::unique_ptr<ast::IfStatement>> ParseIfStmt(Context &ctx,
                                                             TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::If));
    ast::If if_kw(ts.CurrToken()->span());
    ts.Advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LParen));
    ast::LParen lparen(ts.CurrToken()->span());
    ts.Advance();

    auto cond = ParseExpr(ctx, ts);
    if (!cond) return std::nullopt;

    TRY(check_punct(ctx, ts, PunctTokenKind::RParen));
    ast::RParen rparen(ts.CurrToken()->span());
    ts.Advance();

    auto body = ParseStmt(ctx, ts);
    if (!body) return std::nullopt;

    std::optional<ast::IfStatementElseClause> else_clause;
    if (ts && ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Else)) {
        ast::Else else_kw(ts.CurrToken()->span());
        ts.Advance();

        auto else_body = ParseStmt(ctx, ts);
        if (!else_body) return std::nullopt;

        else_clause.emplace(else_kw, std::move(*else_body));
    }

    return std::make_unique<ast::IfStatement>(if_kw, lparen, std::move(*cond),
                                              rparen, std::move(*body),
                                              std::move(else_clause));
}

std::optional<std::unique_ptr<ast::BlockStatement>> ParseBlockStmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_punct(ctx, ts, PunctTokenKind::LCurly));
    ast::LCurly lcurly(ts.CurrToken()->span());
    ts.Advance();

    std::vector<ast::BlockStatementItem> items;
    while (true) {
        TRY(check_eos(ctx, ts));
        if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Let)) {
            ast::Let let_kw(ts.CurrToken()->span());
            ts.Advance();

            std::vector<ast::VariableDeclarationBody> names;
            while (true) {
                TRY(check_ident(ctx, ts));
                std::string value = ts.CurrToken()->IdentValue();
                ast::VariableName name(std::move(value),
                                       ts.CurrToken()->span());
                ts.Advance();

                TRY(check_punct(ctx, ts, PunctTokenKind::Colon));
                ast::Colon colon(ts.CurrToken()->span());
                ts.Advance();

                auto type = ParseType(ctx, ts);
                if (!type) return std::nullopt;

                std::optional<ast::VariableInit> init;
                if (ts && ts.CurrToken()->IsPunctOf(PunctTokenKind::Assign)) {
                    ast::Assign assign(ts.CurrToken()->span());
                    ts.Advance();

                    auto expr = ParseExpr(ctx, ts);
                    if (!expr) return std::nullopt;

                    init.emplace(assign, std::move(*expr));
                }

                names.emplace_back(std::move(name), colon, std::move(*type),
                                   std::move(init));

                TRY(check_eos(ctx, ts));
                if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Comma)) {
                    ts.Advance();
                } else {
                    break;
                }
            }

            TRY(check_punct(ctx, ts, PunctTokenKind::Semicolon));
            ast::Semicolon semicolon(ts.CurrToken()->span());
            ts.Advance();

            items.emplace_back(
                ast::VariableDeclarations(let_kw, std::move(names), semicolon));
        } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::RCurly)) {
            ast::RCurly rcurly(ts.CurrToken()->span());
            ts.Advance();
            return std::make_unique<ast::BlockStatement>(
                lcurly, std::move(items), rcurly);
        } else {
            auto stmt = ParseStmt(ctx, ts);
            if (!stmt) return std::nullopt;
            items.emplace_back(std::move(*stmt));
        }
    }
}

std::optional<std::unique_ptr<ast::Declaration>> ParseDecl(Context &ctx,
                                                           TokenStream &ts) {
    if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Function)) {
        return ParseFuncDecl(ctx, ts);
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Struct)) {
        return ParseStructDecl(ctx, ts);
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Enum)) {
        return ParseEnumDecl(ctx, ts);
    } else {
        ReportInfo info(ts.CurrToken()->span(),
                        "expected one of `function`, `struct` or `enum`", "");
        Report(ctx, ReportLevel::Error, info);
        return std::nullopt;
    }
}

std::optional<std::unique_ptr<ast::FunctionDeclaration>> ParseFuncDecl(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Function));
    ast::Function function_kw(ts.CurrToken()->span());
    ts.Advance();

    TRY(check_ident(ctx, ts));
    std::string value = ts.CurrToken()->IdentValue();
    ast::FunctionDeclarationName name(std::move(value), ts.CurrToken()->span());
    ts.Advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LParen));
    ast::LParen lparen(ts.CurrToken()->span());
    ts.Advance();

    TRY(check_eos(ctx, ts));
    std::vector<ast::FunctionDeclarationParameter> params;
    if (!ts.CurrToken()->IsPunctOf(PunctTokenKind::RParen)) {
        while (true) {
            TRY(check_eos(ctx, ts));
            if (ts.CurrToken()->IsIdent()) {
                std::string value = ts.CurrToken()->IdentValue();
                ast::FunctionDeclarationParameterName name(
                    std::move(value), ts.CurrToken()->span());
                ts.Advance();

                TRY(check_punct(ctx, ts, PunctTokenKind::Colon));
                ast::Colon colon(ts.CurrToken()->span());
                ts.Advance();

                auto type = ParseType(ctx, ts);
                if (!type) return std::nullopt;

                params.emplace_back(std::move(name), colon, std::move(*type));

                TRY(check_eos(ctx, ts));
                if (ts.CurrToken()->IsPunctOf(PunctTokenKind::RParen)) {
                    break;
                } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Comma)) {
                    ts.Advance();
                }
            } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::RParen) &&
                       params.empty()) {
                break;
            } else {
                ReportInfo info(ts.CurrToken()->span(),
                                "unexpected token found",
                                "expected identifier or `)`");
                Report(ctx, ReportLevel::Error, info);
                return std::nullopt;
            }
        }
    }

    TRY(check_punct(ctx, ts, PunctTokenKind::RParen));
    ast::RParen rparen(ts.CurrToken()->span());
    ts.Advance();

    std::optional<ast::FunctionDeclarationReturn> ret;
    if (ts && ts.CurrToken()->IsPunctOf(PunctTokenKind::Arrow)) {
        ast::Arrow arrow(ts.CurrToken()->span());
        ts.Advance();

        auto type = ParseType(ctx, ts);
        if (!type) return std::nullopt;

        ret.emplace(arrow, std::move(*type));
    }

    auto body = ParseBlockStmt(ctx, ts);
    if (!body) return std::nullopt;

    return std::make_unique<ast::FunctionDeclaration>(
        function_kw, std::move(name), lparen, std::move(params), rparen,
        std::move(ret), std::move(*body));
}

std::optional<std::unique_ptr<ast::StructDeclaration>> ParseStructDecl(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Struct));
    ast::Struct struct_kw(ts.CurrToken()->span());
    ts.Advance();

    TRY(check_ident(ctx, ts));
    std::string value = ts.CurrToken()->IdentValue();
    ast::StructDeclarationName name(std::move(value), ts.CurrToken()->span());
    ts.Advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LCurly));
    ast::LCurly lcurly(ts.CurrToken()->span());
    ts.Advance();

    std::vector<ast::StructDeclarationField> fields;
    while (true) {
        TRY(check_eos(ctx, ts));
        if (ts.CurrToken()->IsIdent()) {
            std::string value = ts.CurrToken()->IdentValue();
            ast::StructDeclarationFieldName name(std::move(value),
                                                 ts.CurrToken()->span());
            ts.Advance();

            TRY(check_punct(ctx, ts, PunctTokenKind::Colon));
            ast::Colon colon(ts.CurrToken()->span());
            ts.Advance();

            auto type = ParseType(ctx, ts);
            if (!type) return std::nullopt;

            fields.emplace_back(std::move(name), colon, std::move(*type));

            if (ts && ts.CurrToken()->IsPunctOf(PunctTokenKind::Comma)) {
                ts.Advance();
            } else {
                break;
            }
        } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::RCurly)) {
            break;
        } else {
            if (ts.HasPrev()) {
                ReportInfo info(ts.PrevToken()->span(), "unexpected token",
                                "expected identifier or `}` after this");
                Report(ctx, ReportLevel::Error, info);
                return std::nullopt;
            } else {
                ReportInfo info(ts.CurrToken()->span(), "unexpected token",
                                "expected this to be identifier or `}`");
                Report(ctx, ReportLevel::Error, info);
                return std::nullopt;
            }
        }
    }

    TRY(check_punct(ctx, ts, PunctTokenKind::RCurly));
    ast::RCurly rcurly(ts.CurrToken()->span());
    ts.Advance();

    return std::make_unique<ast::StructDeclaration>(
        struct_kw, std::move(name), lcurly, std::move(fields), rcurly);
}

std::optional<std::unique_ptr<ast::EnumDeclaration>> ParseEnumDecl(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Enum));
    ast::Enum enum_kw(ts.CurrToken()->span());
    ts.Advance();

    TRY(check_ident(ctx, ts));
    std::string value = ts.CurrToken()->IdentValue();
    ast::EnumDeclarationName name(std::move(value), ts.CurrToken()->span());
    ts.Advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LCurly));
    ast::LCurly lcurly(ts.CurrToken()->span());
    ts.Advance();

    std::vector<ast::EnumDeclarationField> fields;
    while (true) {
        TRY(check_eos(ctx, ts));
        if (ts.CurrToken()->IsIdent()) {
            ast::EnumDeclarationFieldName name(
                std::string(ts.CurrToken()->IdentValue()),
                ts.CurrToken()->span());
            ts.Advance();

            std::optional<ast::EnumDeclarationFieldInit> init;
            if (ts && ts.CurrToken()->IsPunctOf(PunctTokenKind::Assign)) {
                ast::Assign assign(ts.CurrToken()->span());
                ts.Advance();

                auto value = ParseLogicalOrExpr(ctx, ts);
                if (!value) return std::nullopt;

                init.emplace(assign, std::move(*value));
            }

            fields.emplace_back(std::move(name), std::move(init));

            if (ts && ts.CurrToken()->IsPunctOf(PunctTokenKind::Comma)) {
                ts.Advance();
            } else {
                break;
            }
        } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::RCurly)) {
            break;
        } else {
            if (ts.HasPrev()) {
                ReportInfo info(ts.PrevToken()->span(), "unexpected token",
                                "expected identifier or `}` after this");
                Report(ctx, ReportLevel::Error, info);
                return std::nullopt;
            } else {
                ReportInfo info(ts.CurrToken()->span(), "unexpected token",
                                "expected this to be identifier or `}`");
                Report(ctx, ReportLevel::Error, info);
                return std::nullopt;
            }
        }
    }

    TRY(check_punct(ctx, ts, PunctTokenKind::RCurly));
    ast::RCurly rcurly(ts.CurrToken()->span());
    ts.Advance();

    return std::make_unique<ast::EnumDeclaration>(
        enum_kw, std::move(name), lcurly, std::move(fields), rcurly);
}

ParserResult ParseFile(Context &ctx, const std::string &path) {
    auto tokens = LexFile(ctx, path);
    if (!tokens) return std::nullopt;
    TokenStream ts(std::move(*tokens));

    std::vector<std::unique_ptr<ast::Declaration>> res;
    while (ts) {
        auto decl = ParseDecl(ctx, ts);
        if (!decl.has_value()) return std::nullopt;
        res.emplace_back(std::move(*decl));
    }
    return res;
}
};  // namespace mini
