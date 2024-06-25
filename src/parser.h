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

using TokenStreamState = size_t;

class TokenStream {
public:
    TokenStream(std::vector<std::unique_ptr<Token>>&& tokens)
        : offset_(0), tokens_(std::move(tokens)) {}
    bool is_eos() const { return offset_ >= tokens_.size(); }
    void advance() { offset_++; }
    const std::unique_ptr<Token>& token() const { return tokens_.at(offset_); }
    bool has_prev() const { return offset_ > 0; }
    const std::unique_ptr<Token>& prev() const {
        return tokens_.at(offset_ - 1);
    }
    const std::unique_ptr<Token>& last() const {
        return tokens_.at(tokens_.size() - 1);
    }
    TokenStreamState state() const { return offset_; }
    void set_state(TokenStreamState state) { offset_ = state; }

private:
    size_t offset_;
    const std::vector<std::unique_ptr<Token>> tokens_;
};

using ParserResult = std::optional<std::vector<std::unique_ptr<Declaration>>>;

std::optional<std::unique_ptr<Expression>> parse_expr(Context& ctx,
                                                      TokenStream& ts);
std::optional<std::unique_ptr<Expression>> parse_logical_or_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<Expression>> parse_logical_and_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<Expression>> parse_inclusive_or_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<Expression>> parse_exclusive_or_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<Expression>> parse_and_expr(Context& ctx,
                                                          TokenStream& ts);
std::optional<std::unique_ptr<Expression>> parse_equality_expr(Context& ctx,
                                                               TokenStream& ts);
std::optional<std::unique_ptr<Expression>> parse_relational_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<Expression>> parse_shift_expr(Context& ctx,
                                                            TokenStream& ts);
std::optional<std::unique_ptr<Expression>> parse_additive_expr(Context& ctx,
                                                               TokenStream& ts);
std::optional<std::unique_ptr<Expression>> parse_multiplicative_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<Expression>> parse_cast_expr(Context& ctx,
                                                           TokenStream& ts);
std::optional<std::unique_ptr<Expression>> parse_unary_expr(Context& ctx,
                                                            TokenStream& ts);
std::optional<std::unique_ptr<Expression>> parse_postfix_expr(Context& ctx,
                                                              TokenStream& ts);
std::optional<std::unique_ptr<Expression>> parse_primary_expr(Context& ctx,
                                                              TokenStream& ts);

std::optional<std::unique_ptr<Statement>> parse_stmt(Context& ctx,
                                                     TokenStream& ts);
std::optional<std::unique_ptr<ExpressionStatement>> parse_expr_stmt(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ReturnStatement>> parse_return_stmt(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<BreakStatement>> parse_break_stmt(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ContinueStatement>> parse_continue_stmt(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<WhileStatement>> parse_while_stmt(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<IfStatement>> parse_if_stmt(Context& ctx,
                                                          TokenStream& ts);
std::optional<std::unique_ptr<BlockStatement>> parse_block_stmt(
    Context& ctx, TokenStream& ts);

std::optional<std::unique_ptr<Type>> parse_type(Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ArrayType>> parse_array_type(Context& ctx,
                                                           TokenStream& ts);

std::optional<std::unique_ptr<Declaration>> parse_decl(Context& ctx,
                                                       TokenStream& ts);
std::optional<std::unique_ptr<FunctionDeclaration>> parse_func_decl(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<StructDeclaration>> parse_struct_decl(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<EnumDeclaration>> parse_enum_decl(
    Context& ctx, TokenStream& ts);

ParserResult parse_file(Context& ctx, const std::string& path);

#endif  // MINI_PARSER_H_
