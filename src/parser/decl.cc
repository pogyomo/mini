#include "decl.h"

#include <memory>

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
    } else if (ts.CurrToken()->IsKeywordOf(KeywordTokenKind::Import)) {
        return ParseImportDecl(ctx, ts);
    } else {
        ReportInfo info(
            ts.CurrToken()->span(),
            "expected one of `function`, `struct`, `enum` or `import`", "");
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

static std::optional<std::unique_ptr<ast::ImportDeclarationImportedMultiItems>>
ParseImportedMultiItems(Context &ctx, TokenStream &ts) {
    ast::LCurly lcurly(ts.CurrToken()->span());
    ts.Advance();

    std::vector<ast::ImportDeclarationImportedSingleItem> items;
    while (true) {
        TRY(check_eos(ctx, ts));
        if (ts.CurrToken()->IsPunctOf(PunctTokenKind::RCurly)) {
            break;
        } else if (ts.CurrToken()->IsIdent()) {
            auto name = ts.CurrToken()->IdentValue();
            items.emplace_back(std::move(name), ts.CurrToken()->span());
            ts.Advance();

            TRY(check_eos(ctx, ts));
            if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Comma)) {
                ts.Advance();
                continue;
            } else if (ts.CurrToken()->IsPunctOf(PunctTokenKind::RCurly)) {
                break;
            } else {
                ReportInfo info(ts.CurrToken()->span(), "unexpected token",
                                "expected this to be `,` or `}`");
                Report(ctx, ReportLevel::Error, info);
                return std::nullopt;
            }
        } else {
            ReportInfo info(ts.CurrToken()->span(), "unexpected token",
                            "expected this to be `}` or identifier");
            Report(ctx, ReportLevel::Error, info);
            return std::nullopt;
        }
    }

    TRY(check_punct(ctx, ts, PunctTokenKind::RCurly));
    ast::RCurly rcurly(ts.CurrToken()->span());
    ts.Advance();

    return std::make_unique<ast::ImportDeclarationImportedMultiItems>(
        lcurly, std::move(items), rcurly);
}

static std::optional<ast::ImportDeclarationPath> ParseImportPath(
    Context &ctx, TokenStream &ts) {
    TRY(check_ident(ctx, ts));
    ast::ImportDeclarationPathItemName name(
        std::string(ts.CurrToken()->IdentValue()), ts.CurrToken()->span());
    ts.Advance();

    TRY(check_eos(ctx, ts));
    if (!ts.CurrToken()->IsPunctOf(PunctTokenKind::Dot)) {
        ast::ImportDeclarationPathItem head(std::move(name), std::nullopt);
        return ast::ImportDeclarationPath(std::move(head), {});
    }
    ast::Dot dot(ts.CurrToken()->span());
    ast::ImportDeclarationPathItem head(std::move(name), std::nullopt);
    ts.Advance();

    std::vector<ast::ImportDeclarationPathItem> rest;
    while (ts) {
        if (ts.CurrToken()->IsIdent()) {
            ast::ImportDeclarationPathItemName name(
                std::string(ts.CurrToken()->IdentValue()),
                ts.CurrToken()->span());
            ts.Advance();

            TRY(check_eos(ctx, ts));
            if (ts.CurrToken()->IsPunctOf(PunctTokenKind::Dot)) {
                ast::Dot dot(ts.CurrToken()->span());
                rest.emplace_back(std::move(name), dot);
            } else {
                rest.emplace_back(std::move(name), std::nullopt);
                break;
            }
        } else {
            ReportInfo info(ts.CurrToken()->span(), "unexpected token",
                            "expected this to be identifier");
            Report(ctx, ReportLevel::Error, info);
            return std::nullopt;
        }
    }
    return ast::ImportDeclarationPath(std::move(head), std::move(rest));
}

std::optional<std::unique_ptr<ast::ImportDeclaration>> ParseImportDecl(
    Context &ctx, TokenStream &ts) {
    TRY(check_keyword(ctx, ts, KeywordTokenKind::Import));
    ast::Import import_kw(ts.CurrToken()->span());
    ts.Advance();

    std::unique_ptr<ast::ImportDeclarationImportedItem> item;
    TRY(check_eos(ctx, ts));
    if (ts.CurrToken()->IsPunctOf(PunctTokenKind::LCurly)) {
        auto maybe_item = ParseImportedMultiItems(ctx, ts);
        if (!maybe_item) return std::nullopt;
        item = std::move(maybe_item.value());
    } else {
        if (ts.CurrToken()->IsIdent()) {
            auto name = ts.CurrToken()->IdentValue();
            item = std::make_unique<ast::ImportDeclarationImportedSingleItem>(
                std::move(name), ts.CurrToken()->span());
            ts.Advance();
        } else {
            ReportInfo info(ts.CurrToken()->span(), "unexpected token",
                            "expected this to be identifier");
            Report(ctx, ReportLevel::Error, info);
            return std::nullopt;
        }
    }

    TRY(check_keyword(ctx, ts, KeywordTokenKind::From));
    ast::From from_kw(ts.CurrToken()->span());
    ts.Advance();

    auto maybe_path = ParseImportPath(ctx, ts);
    if (!maybe_path) return std::nullopt;

    TRY(check_punct(ctx, ts, PunctTokenKind::Semicolon));
    ast::Semicolon semicolon(ts.CurrToken()->span());
    ts.Advance();

    return std::make_unique<ast::ImportDeclaration>(
        import_kw, std::move(item), from_kw, std::move(maybe_path.value()),
        semicolon);
}

}  // namespace mini
