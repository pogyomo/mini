#include "expr.h"

#include "../report.h"
#include "type.h"
#include "utils.h"

namespace mini {

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

        auto expr = ParseUnaryExpr(ctx, ts);
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
            ast::EnumSelectExpressionDst dst(std::move(value2), span2);
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
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::NullPtr)) {
        auto span = ts.CurrToken()->span();
        ts.Advance();

        return std::make_unique<ast::NullPtrExpression>(span);
    } else {
        ReportInfo info(ts.CurrToken()->span(), "unexpected token found",
                        "expected identifier, integer or `(`");
        Report(ctx, ReportLevel::Error, info);
        return std::nullopt;
    }
}

}  // namespace mini
