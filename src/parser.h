#ifndef MINI_PARSER_H_
#define MINI_PARSER_H_

#include <memory>
#include <optional>
#include <vector>

#include "ast/decl.h"
#include "ast/expr.h"
#include "ast/stmt.h"
#include "ast/type.h"
#include "context.h"
#include "token.h"

namespace mini {

using TokenStreamState = size_t;

class TokenStream {
public:
    TokenStream(std::vector<std::unique_ptr<Token>>&& tokens)
        : offset_(0), tokens_(std::move(tokens)) {}
    explicit operator bool() { return offset_ < tokens_.size(); }
    void Advance() { offset_++; }
    const std::unique_ptr<Token>& CurrToken() const {
        return tokens_.at(offset_);
    }
    bool HasPrev() const { return offset_ > 0; }
    const std::unique_ptr<Token>& PrevToken() const {
        return tokens_.at(offset_ - 1);
    }
    const std::unique_ptr<Token>& Last() const {
        return tokens_.at(tokens_.size() - 1);
    }
    TokenStreamState State() const { return offset_; }
    void SetState(TokenStreamState state) { offset_ = state; }

private:
    size_t offset_;
    const std::vector<std::unique_ptr<Token>> tokens_;
};

using ParserResult =
    std::optional<std::vector<std::unique_ptr<ast::Declaration>>>;

std::optional<std::unique_ptr<ast::Expression>> ParseExpr(Context& ctx,
                                                          TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> ParseLogicalOrExpr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> ParseLogicalAndExpr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> ParseInclusiveOrExpr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> ParseExclusiveOrExpr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> ParseAndExpr(Context& ctx,
                                                             TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> ParseEqualityExpr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> ParseRelationalExpr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> ParseShiftExpr(Context& ctx,
                                                               TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> ParseAdditiveExpr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> ParseMultiplicativeExpr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> ParseCastExpr(Context& ctx,
                                                              TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> ParseUnaryExpr(Context& ctx,
                                                               TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> ParsePostfixExpr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> ParsePrimaryExpr(
    Context& ctx, TokenStream& ts);

std::optional<std::unique_ptr<ast::Statement>> ParseStmt(Context& ctx,
                                                         TokenStream& ts);
std::optional<std::unique_ptr<ast::ExpressionStatement>> ParseExprStmt(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::ReturnStatement>> ParseReturnStmt(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::BreakStatement>> ParseBreakStmt(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::ContinueStatement>> ParseContinueStmt(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::WhileStatement>> ParseWhileStmt(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::IfStatement>> ParseIfStmt(Context& ctx,
                                                             TokenStream& ts);
std::optional<std::unique_ptr<ast::BlockStatement>> ParseBlockStmt(
    Context& ctx, TokenStream& ts);

std::optional<std::unique_ptr<ast::Type>> ParseType(Context& ctx,
                                                    TokenStream& ts);
std::optional<std::unique_ptr<ast::ArrayType>> ParseArrayType(Context& ctx,
                                                              TokenStream& ts);

std::optional<std::unique_ptr<ast::Declaration>> ParseDecl(Context& ctx,
                                                           TokenStream& ts);
std::optional<std::unique_ptr<ast::FunctionDeclaration>> ParseFuncDecl(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::StructDeclaration>> ParseStructDecl(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::EnumDeclaration>> ParseEnumDecl(
    Context& ctx, TokenStream& ts);

ParserResult ParseFile(Context& ctx, const std::string& path);

};  // namespace mini

#endif  // MINI_PARSER_H_
