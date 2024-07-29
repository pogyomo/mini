#include "type.h"

#include "../report.h"
#include "expr.h"
#include "utils.h"

namespace mini {

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
                                            lsquare, std::move(size), rsquare);
}

}  // namespace mini
