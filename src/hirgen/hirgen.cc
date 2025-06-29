#include "hirgen.h"

#include "../cflow.h"
#include "../parser/parser.h"
#include "decl.h"

namespace mini {

HirGenResult HirGenFile(Context &ctx, const std::string &path) {
    auto ast_decls = ParseFile(ctx, path);
    if (!ast_decls) return std::nullopt;

    hir::StringTable table;
    HirGenContext gen_ctx(ctx, table);

    for (const auto &decl : ast_decls.value()) {
        DeclVarReg reg(gen_ctx);
        decl->Accept(reg);
    }

    std::vector<std::unique_ptr<hir::Declaration>> decls;
    for (const auto &decl : ast_decls.value()) {
        DeclHirGen gen(gen_ctx);
        decl->Accept(gen);
        if (!gen) return std::nullopt;

        ControlFlowChecker check(ctx);
        gen.decl()->Accept(check);
        if (!check) return std::nullopt;
        decls.emplace_back(std::move(gen.decl()));
    }
    return hir::Root(std::move(table), std::move(decls));
}

}  // namespace mini
