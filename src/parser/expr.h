#ifndef MINI_PARSER_EXPR_H_
#define MINI_PARSER_EXPR_H_

#include <optional>

#include "../ast/expr.h"
#include "../context.h"
#include "stream.h"

namespace mini {

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

}  // namespace mini

#endif  // MINI_PARSER_EXPR_H_
