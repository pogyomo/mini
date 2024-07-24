#include "decl.h"

#include <string>
#include <utility>

#include "context.h"
#include "stmt.h"
#include "type.h"

namespace mini {

static bool ConstructLVarTable(CodeGenContext &ctx,
                               const hir::FunctionDeclaration &decl) {
    ctx.lvar_table().Clear();
    ctx.lvar_table().ChangeSize(0);

    for (const auto &decl : decl.decls()) {
        TypeSizeCalc size(ctx);
        decl.type()->Accept(size);
        if (!size) return false;

        TypeAlignCalc align(ctx);
        decl.type()->Accept(align);
        if (!align) return false;

        ctx.lvar_table().AlignSize(align.align());

        LVarTable::Entry entry(ctx.lvar_table().size());
        ctx.lvar_table().Insert(std::string(decl.name().value()),
                                std::move(entry));

        ctx.lvar_table().AddSize(size.size());
    }

    ctx.lvar_table().AlignSize(16);

    return true;
}

void DeclCollect::Visit(const hir::StructDeclaration &decl) {
    StructTable::Entry entry(decl.span());
    for (const auto &field : decl.fields()) {
        entry.Insert(std::string(field.name().value()), field.type());
    }
    ctx_.struct_table().Insert(std::string(decl.name().value()),
                               std::move(entry));
}

void DeclCollect::Visit(const hir::EnumDeclaration &decl) {
    EnumTable::Entry entry(decl.span());
    for (const auto &field : decl.fields()) {
        entry.Insert(std::string(field.name().value()), field.value().value());
    }
    ctx_.enum_table().Insert(std::string(decl.name().value()),
                             std::move(entry));
}

void DeclCollect::Visit(const hir::FunctionDeclaration &decl) {
    FuncSigTable::Entry entry(decl.ret(), decl.span());
    for (const auto &param : decl.params()) {
        entry.InsertParam(std::string(param.name().value()), param.type());
    }
    ctx_.func_sig_table().Insert(std::string(decl.name().value()),
                                 std::move(entry));
}

void DeclCodeGen::Visit(const hir::FunctionDeclaration &decl) {
    if (!ConstructLVarTable(ctx_, decl)) {
        return;
    }

    ctx_.printer().PrintLn("  .text");
    ctx_.printer().PrintLn("  .type {}, @function", decl.name().value());
    ctx_.printer().PrintLn("  .global {}", decl.name().value());
    ctx_.printer().PrintLn("{}:", decl.name().value());
    ctx_.printer().PrintLn("  pushq %rbp");
    ctx_.printer().PrintLn("  movq %rsp, %rbp");
    ctx_.printer().PrintLn("  subq ${}, %rsp", ctx_.lvar_table().size());

    StmtCodeGen gen(ctx_);
    decl.body().Accept(gen);
    if (!gen) return;

    ctx_.printer().PrintLn("  movq %rbp, %rsp");
    ctx_.printer().PrintLn("  popq %rbp");
    ctx_.printer().PrintLn("  retq");

    success_ = true;
}

}  // namespace mini
