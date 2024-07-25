#include "decl.h"

#include <cstdint>
#include <string>
#include <utility>

#include "../report.h"
#include "context.h"
#include "stmt.h"
#include "type.h"

namespace mini {

void DeclCollect::Visit(const hir::StructDeclaration &decl) {
    StructTable::Entry entry(decl.span());
    for (const auto &field : decl.fields()) {
        if (entry.Exists(field.name().value())) {
            ReportInfo info(field.span(), "duplicated field", "");
            Report(ctx_.ctx(), ReportLevel::Error, info);
            return;
        }
        entry.Insert(std::string(field.name().value()), field.type());
    }
    ctx_.struct_table().Insert(std::string(decl.name().value()),
                               std::move(entry));
    success_ = true;
}

void DeclCollect::Visit(const hir::EnumDeclaration &decl) {
    EnumTable::Entry entry(decl.span());
    for (const auto &field : decl.fields()) {
        if (entry.Exists(field.name().value())) {
            ReportInfo info(field.span(), "duplicated field", "");
            Report(ctx_.ctx(), ReportLevel::Error, info);
            return;
        }
        entry.Insert(std::string(field.name().value()), field.value().value());
    }
    ctx_.enum_table().Insert(std::string(decl.name().value()),
                             std::move(entry));
    success_ = true;
}

void DeclCollect::Visit(const hir::FunctionDeclaration &decl) {
    FuncInfoTable::Entry entry(decl.ret(), decl.span());
    for (const auto &param : decl.params()) {
        entry.params().Insert(std::string(param.name().value()), param.type());
    }
    ctx_.func_info_table().Insert(std::string(decl.name().value()),
                                  std::move(entry));
    success_ = true;
}

void DeclCodeGen::Visit(const hir::FunctionDeclaration &decl) {
    if (!ConstructLVarTable(ctx_, decl)) {
        return;
    }

    ctx_.SetCurrFuncName(std::string(decl.name().value()));

    auto callee_size = ctx_.func_info_table()
                           .Query(decl.name().value())
                           .lvar_table()
                           .CalleeSize();

    ctx_.printer().PrintLn("  .text");
    ctx_.printer().PrintLn("  .type {}, @function", decl.name().value());
    ctx_.printer().PrintLn("  .global {}", decl.name().value());
    ctx_.printer().PrintLn("{}:", decl.name().value());
    ctx_.printer().PrintLn("  pushq %rbp");
    ctx_.printer().PrintLn("  movq %rsp, %rbp");
    ctx_.printer().PrintLn("  subq ${}, %rsp", callee_size);

    // Copy arguments passed by register to stack so that these can take its
    // address.
    auto &params = ctx_.func_info_table().Query(decl.name().value()).params();
    for (const auto &[name, type] : params) {
        auto &lvar = ctx_.lvar_table().Query(name);
        if (lvar.ShouldInitializeWithReg()) {
            auto src = lvar.InitRegName();
            auto dst = lvar.AsmRepr();
            ctx_.printer().PrintLn("  movq {}, {}", src, dst);
        }
    }

    StmtCodeGen gen(ctx_);
    decl.body().Accept(gen);
    if (!gen) return;

    ctx_.printer().PrintLn("  movq %rbp, %rsp");
    ctx_.printer().PrintLn("  popq %rbp");
    ctx_.printer().PrintLn("  retq");

    success_ = true;
}

bool ConstructLVarTable(CodeGenContext &ctx,
                        const hir::FunctionDeclaration &decl) {
    auto &entry = ctx.func_info_table().Query(decl.name().value());
    auto &table = entry.lvar_table();
    table.Clear();
    table.ChangeCalleeSize(0);
    table.ChangeCallerSize(0);

    // Calculate stack memory of caller and callee for arguments.
    uint8_t regnum = 0;
    for (const auto &param : decl.params()) {
        TypeSizeCalc size(ctx);
        param.type()->Accept(size);
        if (!size) return false;

        TypeAlignCalc align(ctx);
        param.type()->Accept(align);
        if (!align) return false;

        if (size.size() <= 8 && regnum < 6) {
            // If the arguments is small enough to place it to register and
            // unused register exists, assign the argument to available
            // register, then allocate callee memory to store it.

            table.AlignCalleeSize(align.align());

            LVarTable::Entry entry(LVarTable::Entry::CalleeAllocArg, regnum,
                                   table.CalleeSize(), param.type());
            table.Insert(std::string(param.name().value()), std::move(entry));

            table.AddCalleeSize(size.size());
            regnum++;
        } else {
            // If the argument is too big to store to register, or no unused
            // register exists, allocate caller stack to store it.

            table.AlignCallerSize(align.align());

            LVarTable::Entry entry(LVarTable::Entry::CallerAllocArg, 0,
                                   table.CallerSize(), param.type());
            table.Insert(std::string(param.name().value()), std::move(entry));

            table.AddCallerSize(size.size());
        }
    }

    // Then, calculate size of stack memory at callee for local variables.
    for (const auto &decl : decl.decls()) {
        TypeSizeCalc size(ctx);
        decl.type()->Accept(size);
        if (!size) return false;

        TypeAlignCalc align(ctx);
        decl.type()->Accept(align);
        if (!align) return false;

        table.AlignCalleeSize(align.align());

        LVarTable::Entry entry(LVarTable::Entry::CalleeLVar, 0,
                               table.CalleeSize(), decl.type());
        table.Insert(std::string(decl.name().value()), std::move(entry));

        table.AddCalleeSize(size.size());
    }

    return true;
}

}  // namespace mini
