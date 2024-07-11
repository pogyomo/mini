#ifndef MINI_PARSER_STMT_H_
#define MINI_PARSER_STMT_H_

#include <memory>
#include <optional>

#include "../ast/stmt.h"
#include "../context.h"
#include "stream.h"

namespace mini {

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

}  // namespace mini

#endif  // MINI_PARSER_STMT_H_
