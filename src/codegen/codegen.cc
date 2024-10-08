#include "codegen.h"

#include "../hirgen/hirgen.h"
#include "context.h"
#include "decl.h"

namespace mini {

bool CodeGenFile(Context &ctx, std::ostream &os, const std::string &path) {
    auto root = HirGenFile(ctx, path);
    if (!root) return false;

    CodeGenContext gen_ctx(ctx, root->string_table(), os);

    // Place string literals to the section `rodata`.
    if (!gen_ctx.string_table().InnerRepr().empty()) {
        gen_ctx.printer().PrintLn("    .section .rodata");
        for (const auto &[s, symbol] : gen_ctx.string_table().InnerRepr()) {
            gen_ctx.printer().PrintLn(".L.{}:", symbol);
            gen_ctx.printer().Print("    .byte ");
            for (const char c : s) {
                gen_ctx.printer().Print("0x{:02x}, ", c);
            }
            gen_ctx.printer().PrintLn("0x00");
        }
    }

    for (const auto &decl : root->decls()) {
        DeclCollect collect(gen_ctx);
        decl->Accept(collect);
        if (!collect) return false;
    }

    for (const auto &decl : root->decls()) {
        DeclCodeGen gen(gen_ctx);
        decl->Accept(gen);
        if (!gen) return false;
    }

    return true;
}

}  // namespace mini
