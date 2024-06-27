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

std::optional<std::unique_ptr<Expression>> parse_expr(Context &ctx,
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
    InfixExpression::Op op(InfixExpression::Op::Kind::Assign,
                           ts.token()->span());
    ts.advance();

    ctx.disable_suppress_report();
    auto rhs = parse_expr(ctx, ts);
    if (!rhs) return std::nullopt;

    return std::make_unique<InfixExpression>(op, std::move(*lhs),
                                             std::move(*rhs));
}

std::optional<std::unique_ptr<Expression>> parse_logical_or_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_logical_and_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::Or)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::Or,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_logical_or_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<Expression>> parse_logical_and_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_inclusive_or_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::And)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::And,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_logical_and_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<Expression>> parse_inclusive_or_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_exclusive_or_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::Vertical)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::BitOr,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_inclusive_or_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<Expression>> parse_exclusive_or_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_and_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::Hat)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::BitXor,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_exclusive_or_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<Expression>> parse_and_expr(Context &ctx,
                                                          TokenStream &ts) {
    auto lhs = parse_equality_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::Ampersand)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::BitAnd,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_and_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<Expression>> parse_equality_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_relational_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::EQ)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::EQ,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_equality_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::NE)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::NE,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_equality_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<Expression>> parse_relational_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_shift_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::LT)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::LT,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_relational_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::LE)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::LE,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_relational_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::GT)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::GT,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_relational_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::GE)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::GE,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_relational_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<Expression>> parse_shift_expr(Context &ctx,
                                                            TokenStream &ts) {
    auto lhs = parse_additive_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::LShift)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::LShift,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_shift_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::RShift)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::RShift,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_shift_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<Expression>> parse_additive_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_multiplicative_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::Plus)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::Add,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_additive_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::Minus)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::Sub,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_additive_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<Expression>> parse_multiplicative_expr(
    Context &ctx, TokenStream &ts) {
    auto lhs = parse_cast_expr(ctx, ts);
    if (!lhs) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::Star)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::Mul,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_multiplicative_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::Slash)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::Div,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_multiplicative_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else if (ts.token()->is_punct_of(PunctTokenKind::Percent)) {
            InfixExpression::Op op(InfixExpression::Op::Kind::Mod,
                                   ts.token()->span());
            ts.advance();
            auto rhs = parse_multiplicative_expr(ctx, ts);
            if (!rhs) return std::nullopt;
            lhs = std::make_unique<InfixExpression>(op, std::move(*lhs),
                                                    std::move(*rhs));
        } else {
            break;
        }
    }
    return lhs;
}

std::optional<std::unique_ptr<Expression>> parse_cast_expr(Context &ctx,
                                                           TokenStream &ts) {
    auto expr = parse_unary_expr(ctx, ts);
    if (!expr) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_keyword_of(KeywordTokenKind::As)) {
            As as_kw(ts.token()->span());
            ts.advance();

            auto type = parse_type(ctx, ts);
            if (!type) return std::nullopt;

            expr = std::make_unique<CastExpression>(std::move(*expr), as_kw,
                                                    std::move(*type));
        } else {
            break;
        }
    }
    return expr;
}

std::optional<std::unique_ptr<Expression>> parse_unary_expr(Context &ctx,
                                                            TokenStream &ts) {
    TRY(check_eos(ctx, ts));
    if (ts.token()->is_punct_of(PunctTokenKind::Ampersand)) {
        UnaryExpression::Op op(UnaryExpression::Op::Kind::Ref,
                               ts.token()->span());
        ts.advance();

        auto expr = parse_unary_expr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<UnaryExpression>(op, std::move(*expr));
    } else if (ts.token()->is_punct_of(PunctTokenKind::Star)) {
        UnaryExpression::Op op(UnaryExpression::Op::Kind::Deref,
                               ts.token()->span());
        ts.advance();

        auto expr = parse_unary_expr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<UnaryExpression>(op, std::move(*expr));
    } else if (ts.token()->is_punct_of(PunctTokenKind::Minus)) {
        UnaryExpression::Op op(UnaryExpression::Op::Kind::Minus,
                               ts.token()->span());
        ts.advance();

        auto expr = parse_unary_expr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<UnaryExpression>(op, std::move(*expr));
    } else if (ts.token()->is_punct_of(PunctTokenKind::Tilde)) {
        UnaryExpression::Op op(UnaryExpression::Op::Kind::Inv,
                               ts.token()->span());
        ts.advance();

        auto expr = parse_unary_expr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<UnaryExpression>(op, std::move(*expr));
    } else if (ts.token()->is_punct_of(PunctTokenKind::Exclamation)) {
        UnaryExpression::Op op(UnaryExpression::Op::Kind::Neg,
                               ts.token()->span());
        ts.advance();

        auto expr = parse_unary_expr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<UnaryExpression>(op, std::move(*expr));
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::ESizeof)) {
        ESizeof esizeof_kw(ts.token()->span());
        ts.advance();

        auto expr = parse_expr(ctx, ts);
        if (!expr) return std::nullopt;

        return std::make_unique<ESizeofExpression>(esizeof_kw,
                                                   std::move(*expr));
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::TSizeof)) {
        TSizeof tsizeof_kw(ts.token()->span());
        ts.advance();

        auto type = parse_type(ctx, ts);
        if (!type) return std::nullopt;

        return std::make_unique<TSizeofExpression>(tsizeof_kw,
                                                   std::move(*type));
    } else {
        return parse_postfix_expr(ctx, ts);
    }
}

std::optional<std::unique_ptr<Expression>> parse_postfix_expr(Context &ctx,
                                                              TokenStream &ts) {
    auto expr = parse_primary_expr(ctx, ts);
    if (!expr) return std::nullopt;
    while (!ts.is_eos()) {
        if (ts.token()->is_punct_of(PunctTokenKind::LSquare)) {
            LSquare lsquare(ts.token()->span());
            ts.advance();

            auto index = parse_expr(ctx, ts);
            if (!index) return std::nullopt;

            TRY(check_punct(ctx, ts, PunctTokenKind::RSquare));
            RSquare rsquare(ts.token()->span());
            ts.advance();

            expr = std::make_unique<IndexExpression>(
                std::move(*expr), lsquare, std::move(*index), rsquare);
        } else if (ts.token()->is_punct_of(PunctTokenKind::LParen)) {
            LParen lparen(ts.token()->span());
            ts.advance();

            std::vector<std::unique_ptr<Expression>> args;
            while (true) {
                TRY(check_eos(ctx, ts));
                if (ts.token()->is_punct_of(PunctTokenKind::RParen) &&
                    args.empty()) {
                    RParen rparen(ts.token()->span());
                    ts.advance();

                    expr = std::make_unique<CallExpression>(
                        std::move(*expr), lparen, std::move(args), rparen);
                    break;
                } else {
                    auto arg = parse_expr(ctx, ts);
                    if (!arg) return std::nullopt;

                    args.emplace_back(std::move(*arg));

                    if (ts.token()->is_punct_of(PunctTokenKind::RParen)) {
                        RParen rparen(ts.token()->span());
                        ts.advance();

                        expr = std::make_unique<CallExpression>(
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
            Dot dot(ts.token()->span());
            ts.advance();

            TRY(check_ident(ctx, ts));
            std::string value = ts.token()->ident_value();
            AccessExpressionField field(std::move(value), ts.token()->span());
            ts.advance();

            expr = std::make_unique<AccessExpression>(std::move(*expr), dot,
                                                      std::move(field));
        } else {
            break;
        }
    }
    return expr;
}

std::optional<std::unique_ptr<Expression>> parse_primary_expr(Context &ctx,
                                                              TokenStream &ts) {
    TRY(check_eos(ctx, ts));
    if (ts.token()->is_ident()) {
        std::string value1 = ts.token()->ident_value();
        auto span1 = ts.token()->span();
        ts.advance();

        if (!ts.is_eos() &&
            ts.token()->is_punct_of(PunctTokenKind::ColonColon)) {
            ColonColon colon_colon(ts.token()->span());
            ts.advance();

            TRY(check_ident(ctx, ts));
            std::string value2 = ts.token()->ident_value();
            auto span2 = ts.token()->span();
            ts.advance();

            EnumSelectExpressionSrc src(std::move(value1), span1);
            EnumSelectExpressionDst dst(std::move(value1), span1);
            return std::make_unique<EnumSelectExpression>(
                std::move(dst), colon_colon, std::move(src));
        } else {
            return std::make_unique<VariableExpression>(std::move(value1),
                                                        span1);
        }
    } else if (ts.token()->is_int()) {
        uint64_t value = ts.token()->int_value();
        auto span = ts.token()->span();
        ts.advance();

        return std::make_unique<IntegerExpression>(value, span);
    } else if (ts.token()->is_punct_of(PunctTokenKind::LParen)) {
        ts.advance();
        auto expr = parse_expr(ctx, ts);
        if (!expr) return std::nullopt;

        TRY(check_punct(ctx, ts, PunctTokenKind::RParen));
        ts.advance();

        return expr;
    } else {
        ReportInfo info(ts.token()->span(), "unexpected token found",
                        "expected identifier, integer or `(`");
        report(ctx, ReportLevel::Error, info);
        return std::nullopt;
    }
}

std::optional<std::unique_ptr<Type>> parse_type(Context &ctx, TokenStream &ts) {
    TRY(check_eos(ctx, ts));
    if (ts.token()->is_keyword_of(KeywordTokenKind::Int)) {
        auto type = std::make_unique<IntType>(ts.token()->span());
        ts.advance();
        return type;
    } else if (ts.token()->is_keyword_of(KeywordTokenKind::UInt)) {
        auto type = std::make_unique<UIntType>(ts.token()->span());
        ts.advance();
        return type;
    } else if (ts.token()->is_punct_of(PunctTokenKind::Star)) {
        Star star(ts.token()->span());
        ts.advance();
        auto of = parse_type(ctx, ts);
        if (!of) return std::nullopt;
        return std::make_unique<PointerType>(star, std::move(*of));
    } else if (ts.token()->is_punct_of(PunctTokenKind::LParen)) {
        return parse_array_type(ctx, ts);
    } else if (ts.token()->is_ident()) {
        auto name = ts.token()->ident_value();
        auto span = ts.token()->span();
        ts.advance();
        return std::make_unique<NameType>(std::move(name), span);
    } else {
        ReportInfo info(ts.token()->span(),
                        "expected one of `int`, `uint`, `(` or identifier", "");
        report(ctx, ReportLevel::Error, info);
        ts.advance();
        return std::nullopt;
    }
}

std::optional<std::unique_ptr<ArrayType>> parse_array_type(Context &ctx,
                                                           TokenStream &ts) {
    TRY(check_punct(ctx, ts, PunctTokenKind::LParen));
    LParen lparen(ts.token()->span());
    ts.advance();

    auto of = parse_type(ctx, ts);
    if (!of) return std::nullopt;

    TRY(check_punct(ctx, ts, PunctTokenKind::RParen));
    RParen rparen(ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LSquare));
    LSquare lsquare(ts.token()->span());
    ts.advance();

    auto size = parse_expr(ctx, ts);
    if (!size) return std::nullopt;

    TRY(check_punct(ctx, ts, PunctTokenKind::RSquare));
    RSquare rsquare(ts.token()->span());
    ts.advance();

    return std::make_unique<ArrayType>(lparen, std::move(*of), rparen, lsquare,
                                       std::move(*size), rsquare);
}

std::optional<std::unique_ptr<Statement>> parse_stmt(Context &ctx,
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

std::optional<std::unique_ptr<ExpressionStatement>> parse_expr_stmt(
    Context &ctx, TokenStream &ts) {
    auto expr = parse_expr(ctx, ts);
    if (!expr) return std::nullopt;

    TRY(check_punct(ctx, ts, PunctTokenKind::Semicolon));
    Semicolon semicolon(ts.token()->span());
    ts.advance();

    return std::make_unique<ExpressionStatement>(std::move(*expr), semicolon);
}

std::optional<std::unique_ptr<ReturnStatement>> parse_return_stmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Return));
    Return return_kw(ts.token()->span());
    ts.advance();

    std::optional<std::unique_ptr<Expression>> expr;
    TRY(check_eos(ctx, ts));
    if (!ts.token()->is_punct_of(PunctTokenKind::Semicolon)) {
        expr = parse_expr(ctx, ts);
        if (!expr) return std::nullopt;
    }

    TRY(check_punct(ctx, ts, PunctTokenKind::Semicolon));
    Semicolon semicolon(ts.token()->span());
    ts.advance();

    return std::make_unique<ReturnStatement>(return_kw, std::move(*expr),
                                             semicolon);
}

std::optional<std::unique_ptr<BreakStatement>> parse_break_stmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Break));
    Break break_kw(ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::Semicolon));
    Semicolon semicolon(ts.token()->span());
    ts.advance();

    return std::make_unique<BreakStatement>(break_kw, semicolon);
}

std::optional<std::unique_ptr<ContinueStatement>> parse_continue_stmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Continue));
    Continue continue_kw(ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::Semicolon));
    Semicolon semicolon(ts.token()->span());
    ts.advance();

    return std::make_unique<ContinueStatement>(continue_kw, semicolon);
}

std::optional<std::unique_ptr<WhileStatement>> parse_while_stmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::While));
    While while_kw(ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LParen));
    LParen lparen(ts.token()->span());
    ts.advance();

    auto cond = parse_expr(ctx, ts);
    if (!cond) return std::nullopt;

    TRY(check_punct(ctx, ts, PunctTokenKind::RParen));
    RParen rparen(ts.token()->span());
    ts.advance();

    auto body = parse_stmt(ctx, ts);
    if (!body) return std::nullopt;

    return std::make_unique<WhileStatement>(while_kw, lparen, std::move(*cond),
                                            rparen, std::move(*body));
}

std::optional<std::unique_ptr<IfStatement>> parse_if_stmt(Context &ctx,
                                                          TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::If));
    If if_kw(ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LParen));
    LParen lparen(ts.token()->span());
    ts.advance();

    auto cond = parse_expr(ctx, ts);
    if (!cond) return std::nullopt;

    TRY(check_punct(ctx, ts, PunctTokenKind::RParen));
    RParen rparen(ts.token()->span());
    ts.advance();

    auto body = parse_stmt(ctx, ts);
    if (!body) return std::nullopt;

    std::optional<IfStatementElseClause> else_clause;
    if (!ts.is_eos() && ts.token()->is_keyword_of(KeywordTokenKind::Else)) {
        Else else_kw(ts.token()->span());
        ts.advance();

        auto else_body = parse_stmt(ctx, ts);
        if (!else_body) return std::nullopt;

        else_clause.emplace(else_kw, std::move(*else_body));
    }

    return std::make_unique<IfStatement>(if_kw, lparen, std::move(*cond),
                                         rparen, std::move(*body),
                                         std::move(else_clause));
}

std::optional<std::unique_ptr<BlockStatement>> parse_block_stmt(
    Context &ctx, TokenStream &ts) {
    TRY(check_punct(ctx, ts, PunctTokenKind::LCurly));
    LCurly lcurly(ts.token()->span());
    ts.advance();

    std::vector<VariableDeclarations> decls;
    while (true) {
        TRY(check_eos(ctx, ts));
        if (ts.token()->is_keyword_of(KeywordTokenKind::Let)) {
            Let let_kw(ts.token()->span());
            ts.advance();

            std::vector<VariableDeclarationBody> names;
            while (true) {
                TRY(check_ident(ctx, ts));
                std::string value = ts.token()->ident_value();
                VariableName name(std::move(value), ts.token()->span());
                ts.advance();

                TRY(check_punct(ctx, ts, PunctTokenKind::Colon));
                Colon colon(ts.token()->span());
                ts.advance();

                auto type = parse_type(ctx, ts);
                if (!type) return std::nullopt;

                std::optional<VariableInit> init;
                if (!ts.is_eos() &&
                    ts.token()->is_punct_of(PunctTokenKind::Assign)) {
                    Assign assign(ts.token()->span());
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
            Semicolon semicolon(ts.token()->span());
            ts.advance();

            decls.emplace_back(let_kw, std::move(names), semicolon);
        } else {
            break;
        }
    }

    std::vector<std::unique_ptr<Statement>> stmts;
    while (true) {
        TRY(check_eos(ctx, ts));
        if (ts.token()->is_punct_of(PunctTokenKind::RCurly)) {
            RCurly rcurly(ts.token()->span());
            ts.advance();
            return std::make_unique<BlockStatement>(lcurly, std::move(decls),
                                                    std::move(stmts), rcurly);
        } else {
            auto stmt = parse_stmt(ctx, ts);
            if (!stmt) return std::nullopt;
            stmts.emplace_back(std::move(*stmt));
        }
    }
}

std::optional<std::unique_ptr<Declaration>> parse_decl(Context &ctx,
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

std::optional<std::unique_ptr<FunctionDeclaration>> parse_func_decl(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Function));
    Function function_kw(ts.token()->span());
    ts.advance();

    TRY(check_ident(ctx, ts));
    std::string value = ts.token()->ident_value();
    FunctionDeclarationName name(std::move(value), ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LParen));
    LParen lparen(ts.token()->span());
    ts.advance();

    TRY(check_eos(ctx, ts));
    std::vector<FunctionDeclarationParameter> params;
    if (!ts.token()->is_punct_of(PunctTokenKind::RParen)) {
        while (true) {
            TRY(check_eos(ctx, ts));
            if (ts.token()->is_ident()) {
                std::string value = ts.token()->ident_value();
                FunctionDeclarationParameterName name(std::move(value),
                                                      ts.token()->span());
                ts.advance();

                TRY(check_punct(ctx, ts, PunctTokenKind::Colon));
                Colon colon(ts.token()->span());
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
    RParen rparen(ts.token()->span());
    ts.advance();

    std::optional<FunctionDeclarationReturn> ret;
    if (!ts.is_eos() && ts.token()->is_punct_of(PunctTokenKind::Arrow)) {
        Arrow arrow(ts.token()->span());
        ts.advance();

        auto type = parse_type(ctx, ts);
        if (!type) return std::nullopt;

        ret.emplace(arrow, std::move(*type));
    }

    auto body = parse_block_stmt(ctx, ts);
    if (!body) return std::nullopt;

    return std::make_unique<FunctionDeclaration>(
        function_kw, std::move(name), lparen, std::move(params), rparen,
        std::move(ret), std::move(*body));
}

std::optional<std::unique_ptr<StructDeclaration>> parse_struct_decl(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Struct));
    Struct struct_kw(ts.token()->span());
    ts.advance();

    TRY(check_ident(ctx, ts));
    std::string value = ts.token()->ident_value();
    StructDeclarationName name(std::move(value), ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LCurly));
    LCurly lcurly(ts.token()->span());
    ts.advance();

    std::vector<StructDeclarationField> fields;
    while (true) {
        TRY(check_eos(ctx, ts));
        if (ts.token()->is_ident()) {
            std::string value = ts.token()->ident_value();
            StructDeclarationFieldName name(std::move(value),
                                            ts.token()->span());
            ts.advance();

            TRY(check_punct(ctx, ts, PunctTokenKind::Colon));
            Colon colon(ts.token()->span());
            ts.advance();

            auto type = parse_type(ctx, ts);
            if (!type) return std::nullopt;

            fields.emplace_back(std::move(name), colon, std::move(*type));

            if (ts.token()->is_punct_of(PunctTokenKind::RCurly)) {
                break;
            } else if (ts.token()->is_punct_of(PunctTokenKind::Comma)) {
                ts.advance();
            } else {
                ReportInfo info(ts.token()->span(), "unexpected token",
                                "expected one of identifier, `}`, or `,`");
                report(ctx, ReportLevel::Error, info);
                return std::nullopt;
            }
        } else if (ts.token()->is_punct_of(PunctTokenKind::RCurly)) {
            break;
        } else {
            ReportInfo info(ts.token()->span(), "unexpected token",
                            "expected identifier or `}`");
            report(ctx, ReportLevel::Error, info);
            return std::nullopt;
        }
    }

    TRY(check_punct(ctx, ts, PunctTokenKind::RCurly));
    RCurly rcurly(ts.token()->span());
    ts.advance();

    return std::make_unique<StructDeclaration>(
        struct_kw, std::move(name), lcurly, std::move(fields), rcurly);
}

std::optional<std::unique_ptr<EnumDeclaration>> parse_enum_decl(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Enum));
    Enum enum_kw(ts.token()->span());
    ts.advance();

    TRY(check_ident(ctx, ts));
    std::string value = ts.token()->ident_value();
    EnumDeclarationName name(std::move(value), ts.token()->span());
    ts.advance();

    TRY(check_punct(ctx, ts, PunctTokenKind::LCurly));
    LCurly lcurly(ts.token()->span());
    ts.advance();

    std::vector<EnumDeclarationField> fields;
    while (true) {
        TRY(check_eos(ctx, ts));
        if (ts.token()->is_ident()) {
            std::string name = ts.token()->ident_value();
            fields.emplace_back(std::move(name), ts.token()->span());
            ts.advance();

            if (ts.token()->is_punct_of(PunctTokenKind::RCurly)) {
                break;
            } else if (ts.token()->is_punct_of(PunctTokenKind::Comma)) {
                ts.advance();
            } else {
                ReportInfo info(ts.token()->span(), "unexpected token",
                                "expected one of identifier, `}`, or `,`");
                report(ctx, ReportLevel::Error, info);
                return std::nullopt;
            }
        } else if (ts.token()->is_punct_of(PunctTokenKind::RCurly)) {
            break;
        } else {
            ReportInfo info(ts.token()->span(), "unexpected token",
                            "expected identifier or `}`");
            report(ctx, ReportLevel::Error, info);
            return std::nullopt;
        }
    }

    TRY(check_punct(ctx, ts, PunctTokenKind::RCurly));
    RCurly rcurly(ts.token()->span());
    ts.advance();

    return std::make_unique<EnumDeclaration>(enum_kw, std::move(name), lcurly,
                                             std::move(fields), rcurly);
}

ParserResult parse_file(Context &ctx, const std::string &path) {
    auto tokens = lex_file(ctx, path);
    if (!tokens) return std::nullopt;
    TokenStream ts(std::move(*tokens));

    std::vector<std::unique_ptr<Declaration>> res;
    while (!ts.is_eos()) {
        auto decl = parse_decl(ctx, ts);
        if (!decl.has_value()) return std::nullopt;
        res.emplace_back(std::move(*decl));
    }
    return res;
}
