#include "stmt.h"

#include "expr.h"
#include "type.h"
#include "utils.h"

namespace mini {

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

}  // namespace mini
