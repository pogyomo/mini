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

using ParserResult =
    std::optional<std::vector<std::unique_ptr<ast::Declaration>>>;

std::optional<std::unique_ptr<ast::Expression>> parse_expr(Context& ctx,
                                                           TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> parse_logical_or_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> parse_logical_and_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> parse_inclusive_or_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> parse_exclusive_or_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> parse_and_expr(Context& ctx,
                                                               TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> parse_equality_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> parse_relational_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> parse_shift_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> parse_additive_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> parse_multiplicative_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> parse_cast_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> parse_unary_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> parse_postfix_expr(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::Expression>> parse_primary_expr(
    Context& ctx, TokenStream& ts);

std::optional<std::unique_ptr<ast::Statement>> parse_stmt(Context& ctx,
                                                          TokenStream& ts);
std::optional<std::unique_ptr<ast::ExpressionStatement>> parse_expr_stmt(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::ReturnStatement>> parse_return_stmt(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::BreakStatement>> parse_break_stmt(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::ContinueStatement>> parse_continue_stmt(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::WhileStatement>> parse_while_stmt(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::IfStatement>> parse_if_stmt(Context& ctx,
                                                               TokenStream& ts);
std::optional<std::unique_ptr<ast::BlockStatement>> parse_block_stmt(
    Context& ctx, TokenStream& ts);

std::optional<std::unique_ptr<ast::Type>> parse_type(Context& ctx,
                                                     TokenStream& ts);
std::optional<std::unique_ptr<ast::ArrayType>> parse_array_type(
    Context& ctx, TokenStream& ts);

std::optional<std::unique_ptr<ast::Declaration>> parse_decl(Context& ctx,
                                                            TokenStream& ts);
std::optional<std::unique_ptr<ast::FunctionDeclaration>> parse_func_decl(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::StructDeclaration>> parse_struct_decl(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::EnumDeclaration>> parse_enum_decl(
    Context& ctx, TokenStream& ts);

ParserResult parse_file(Context& ctx, const std::string& path);

};  // namespace mini

#endif  // MINI_PARSER_H_
