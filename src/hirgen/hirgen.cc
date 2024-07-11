#include "hirgen.h"

#include <memory>
#include <string>
#include <vector>

#include "../ast/decl.h"
#include "../context.h"
#include "../hir/decl.h"
#include "../parser.h"
#include "context.h"
#include "decl.h"

namespace mini {

HirgenResult HirgenFile(Context &ctx, const std::string &path) {
    auto decls = ParseFile(ctx, path);
    if (!decls) return std::nullopt;

    HirGenContext gen_ctx(ctx);

    for (const auto &decl : decls.value()) {
        DeclVarReg reg(gen_ctx);
        decl->Accept(reg);
    }

    std::vector<std::unique_ptr<hir::Declaration>> res;
    for (const auto &decl : decls.value()) {
        DeclHirGen gen(gen_ctx);
        decl->Accept(gen);
        if (!gen) return std::nullopt;
        res.emplace_back(std::move(gen.decl()));
    }
    return res;
}

}  // namespace mini
