#ifndef MINI_PARSER_DECL_H_
#define MINI_PARSER_DECL_H_

#include <memory>
#include <optional>

#include "../ast/decl.h"
#include "../context.h"
#include "stream.h"

namespace mini {

std::optional<std::unique_ptr<ast::Declaration>> ParseDecl(Context& ctx,
                                                           TokenStream& ts);
std::optional<std::unique_ptr<ast::FunctionDeclaration>> ParseFuncDecl(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::StructDeclaration>> ParseStructDecl(
    Context& ctx, TokenStream& ts);
std::optional<std::unique_ptr<ast::EnumDeclaration>> ParseEnumDecl(
    Context& ctx, TokenStream& ts);

}  // namespace mini

#endif  // MINI_PARSER_DECL_H_
