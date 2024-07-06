#ifndef MINI_LEXER_H_
#define MINI_LEXER_H_

#include <memory>
#include <optional>
#include <vector>

#include "context.h"
#include "token.h"

namespace mini {

using LexResult = std::optional<std::vector<std::unique_ptr<Token>>>;

LexResult lex_file(Context& ctx, const std::string& path);

};  // namespace mini

#endif  // MINI_LEXER_H_
