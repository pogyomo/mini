#include "parser.h"

#include "../lexer.h"
#include "decl.h"
#include "stream.h"

namespace mini {

ParserResult ParseFile(Context &ctx, const std::string &path) {
    auto tokens = LexFile(ctx, path);
    if (!tokens) return std::nullopt;
    TokenStream ts(std::move(*tokens));

    std::vector<std::unique_ptr<ast::Declaration>> res;
    while (ts) {
        auto decl = ParseDecl(ctx, ts);
        if (!decl.has_value()) return std::nullopt;
        res.emplace_back(std::move(*decl));
    }
    return res;
}

};  // namespace mini
