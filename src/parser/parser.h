#ifndef MINI_PARSER_H_
#define MINI_PARSER_H_

#include <memory>
#include <optional>
#include <vector>

#include "../ast/decl.h"
#include "../context.h"

namespace mini {

using ParserResult =
    std::optional<std::vector<std::unique_ptr<ast::Declaration>>>;

ParserResult ParseFile(Context& ctx, const std::string& path);

};  // namespace mini

#endif  // MINI_PARSER_H_
