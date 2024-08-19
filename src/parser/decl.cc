#include "decl.h"

#include "../report.h"
#include "expr.h"
#include "stmt.h"
#include "type.h"
#include "utils.h"

namespace mini {

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
    std::vector<ast::FunctionDeclarationParam> params;
    std::optional<ast::FunctionDeclarationVariadic> variadic;
    if (!ts.CurrToken()->IsPunctOf(PunctTokenKind::RParen)) {
        while (true) {
            TRY(check_eos(ctx, ts));
            if (ts.CurrToken()->IsIdent()) {
                std::string value = ts.CurrToken()->IdentValue();
                ast::FunctionDeclarationParamName name(std::move(value),
                                                       ts.CurrToken()->span());
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
            } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::DotDotDot)) {
                ast::DotDotDot dot3(ts.CurrToken()->span());
                variadic.emplace(dot3);
                ts.Advance();
                break;
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

    if (ts && ts.CurrToken()->IsPunctOf(PunctTokenKind::Semicolon)) {
        ast::Semicolon semicolon(ts.CurrToken()->span());
        ts.Advance();

        return std::make_unique<ast::FunctionDeclaration>(
            function_kw, std::move(name), lparen, std::move(params), variadic,
            rparen, std::move(ret), semicolon);
    } else {
        auto body = ParseBlockStmt(ctx, ts);
        if (!body) return std::nullopt;

        return std::make_unique<ast::FunctionDeclaration>(
            function_kw, std::move(name), lparen, std::move(params), variadic,
            rparen, std::move(ret), std::move(*body));
    }
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

}  // namespace mini
