#include "stmt.h"

#include <cassert>
#include <memory>

#include "../report.h"
#include "asm.h"
#include "expr.h"
#include "fmt/base.h"
#include "type.h"

namespace mini {

void StmtCodeGen::Visit(const hir::ExpressionStatement &stmt) {
    ctx_.lvar_table().SaveCalleeSize();

    ExprRValGen gen(ctx_);
    stmt.expr()->Accept(gen);
    if (!gen) return;

    // Free allocated value if exists.
    auto diff = ctx_.lvar_table().RestoreCalleeSize();
    if (diff) ctx_.printer().PrintLn("    addq ${}, %rsp", diff);

    success_ = true;
}

void StmtCodeGen::Visit(const hir::ReturnStatement &stmt) {
    auto &func = ctx_.func_info_table().Query(ctx_.CurrFuncName());
    if (!stmt.ret_value()) {
        if (!func.ret_type()->IsBuiltin() ||
            func.ret_type()->ToBuiltin()->kind() != hir::BuiltinType::Void) {
            ReportInfo info(stmt.span(), "incorrect return type", "");
            Report(ctx_.ctx(), ReportLevel::Error, info);
            return;
        }
    } else {
        ctx_.lvar_table().SaveCalleeSize();

        ExprRValGen gen(ctx_);
        stmt.ret_value().value()->Accept(gen);
        if (!gen) return;

        if (!ImplicitlyConvertValueInStack(ctx_,
                                           stmt.ret_value().value()->span(),
                                           gen.inferred(), func.ret_type())) {
            return;
        }

        TypeSizeCalc size(ctx_);
        gen.inferred()->Accept(size);
        if (!size) return;

        if (size.size() > 8) {
            // Move address to rax.
            ctx_.printer().PrintLn("    movq (%rsp), %rax");

            // Copy fat object.
            IndexableAsmRegPtr src(Register::AX, 0);
            IndexableAsmRegPtr dst(Register::DI, 0);
            CopyBytes(ctx_, src, dst, size.size());
            ctx_.printer().PrintLn("    movq %rdi, %rax");
        } else {
            ctx_.lvar_table().SubCalleeSize(8);
            ctx_.printer().PrintLn("    popq %rax");
        }

        // Free allocated value if exists.
        auto diff = ctx_.lvar_table().RestoreCalleeSize();
        if (diff) ctx_.printer().PrintLn("    addq ${}, %rsp", diff);
    }

    ctx_.printer().PrintLn("    jmp {}.END", ctx_.CurrFuncName());

    success_ = true;
}

void StmtCodeGen::Visit(const hir::BreakStatement &expr) {
    if (!ctx_.IsInLoop()) {
        ReportInfo info(expr.span(), "break used from outside of loop", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    auto id = ctx_.label_id_generator().CurrId();
    ctx_.printer().PrintLn("    jmp L.END.{}", id);
    success_ = true;
}

void StmtCodeGen::Visit(const hir::ContinueStatement &expr) {
    if (!ctx_.IsInLoop()) {
        ReportInfo info(expr.span(), "continue used from outside of loop", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    auto id = ctx_.label_id_generator().CurrId();
    ctx_.printer().PrintLn("    jmp L.START.{}", id);
    success_ = true;
}

void StmtCodeGen::Visit(const hir::WhileStatement &stmt) {
    auto id = ctx_.label_id_generator().GenNewId();

    ctx_.EnterLoop();

    ctx_.printer().PrintLn("L.START.{}:", id);

    ExprRValGen cond_gen(ctx_);
    stmt.cond()->Accept(cond_gen);
    if (!cond_gen) return;

    auto to = std::make_shared<hir::BuiltinType>(hir::BuiltinType::Bool,
                                                 stmt.cond()->span());
    if (!ImplicitlyConvertValueInStack(ctx_, stmt.cond()->span(),
                                       cond_gen.inferred(), to)) {
        return;
    }

    ctx_.lvar_table().SubCalleeSize(8);
    ctx_.printer().PrintLn("    popq %rax");
    ctx_.printer().PrintLn("    test %ax, %ax");
    ctx_.printer().PrintLn("    je L.END.{}", id);

    StmtCodeGen body_gen(ctx_);
    stmt.body()->Accept(body_gen);
    if (!body_gen) return;
    ctx_.printer().PrintLn("    jmp L.START.{}", id);

    ctx_.printer().PrintLn("L.END.{}:", id);

    ctx_.LeaveLoop();

    success_ = true;
}

void StmtCodeGen::Visit(const hir::IfStatement &stmt) {
    auto id = ctx_.label_id_generator().GenNewId();

    ExprRValGen cond_gen(ctx_);
    stmt.cond()->Accept(cond_gen);
    if (!cond_gen) return;

    auto to = std::make_shared<hir::BuiltinType>(hir::BuiltinType::Bool,
                                                 stmt.cond()->span());
    if (!ImplicitlyConvertValueInStack(ctx_, stmt.cond()->span(),
                                       cond_gen.inferred(), to)) {
        return;
    }

    ctx_.lvar_table().SubCalleeSize(8);
    ctx_.printer().PrintLn("    popq %rax");
    ctx_.printer().PrintLn("    test %ax, %ax");
    ctx_.printer().PrintLn("    je L.ELSE.{}", id);

    StmtCodeGen then_gen(ctx_);
    stmt.then_body()->Accept(then_gen);
    if (!then_gen) return;
    ctx_.printer().PrintLn("    jmp L.END.{}", id);

    ctx_.printer().PrintLn("L.ELSE.{}:", id);

    if (stmt.else_body()) {
        StmtCodeGen else_gen(ctx_);
        stmt.else_body().value()->Accept(else_gen);
        if (!else_gen) return;
    }

    ctx_.printer().PrintLn("L.END.{}:", id);

    success_ = true;
}

void StmtCodeGen::Visit(const hir::BlockStatement &stmt) {
    for (const auto &stmt : stmt.stmts()) {
        StmtCodeGen gen(ctx_);
        stmt->Accept(gen);
        if (!gen) return;
    }
    success_ = true;
}

}  // namespace mini
