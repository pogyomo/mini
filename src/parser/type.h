#ifndef MINI_PARSER_TYPE_H_
#define MINI_PARSER_TYPE_H_

#include <memory>
#include <optional>

#include "../ast/type.h"
#include "../context.h"
#include "stream.h"

namespace mini {

std::optional<std::unique_ptr<ast::Type>> ParseType(Context& ctx,
                                                    TokenStream& ts);
std::optional<std::unique_ptr<ast::ArrayType>> ParseArrayType(Context& ctx,
                                                              TokenStream& ts);

}  // namespace mini

#endif  // MINI_PARSER_TYPE_H_
