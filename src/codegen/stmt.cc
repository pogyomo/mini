#include "stmt.h"

#include <cassert>

#include "expr.h"

namespace mini {

void StmtCodeGen::Visit(const hir::ExpressionStatement &stmt) {
    auto size = ctx_.lvar_table().CalleeSize();

    ExprCodeGen gen(ctx_);
    stmt.expr()->Accept(gen);
    if (!gen) return;

    // Free allocated value if exists.
    auto changed_size = ctx_.lvar_table().CalleeSize();
    if (changed_size != size) {
        assert(changed_size > size);
        ctx_.printer().PrintLn("  add ${}, %rsp", changed_size - size);
        ctx_.lvar_table().ChangeCalleeSize(size);
    }

    success_ = true;
}

void StmtCodeGen::Visit(const hir::ReturnStatement &stmt) {
    if (!stmt.ret_value()) {
        auto &func = ctx_.func_info_table().Query(ctx_.CurrFuncName());
        if (func.ret_type()->IsBuiltin() &&
            func.ret_type()->ToBuiltin()->kind() == hir::BuiltinType::Void) {
            ctx_.printer().PrintLn("  retq");
        }
    } else {
        ExprCodeGen gen(ctx_);
        stmt.ret_value().value()->Accept(gen);
        if (!gen) return;

        // TODO: Check types
        ctx_.printer().PrintLn("  retq");
    }
}

void StmtCodeGen::Visit(const hir::BreakStatement &) {
    auto id = ctx_.label_id_generator().CurrId();
    ctx_.printer().PrintLn("  jmp L.END.{}", id);
}

void StmtCodeGen::Visit(const hir::ContinueStatement &) {
    auto id = ctx_.label_id_generator().CurrId();
    ctx_.printer().PrintLn("  jmp L.START.{}", id);
}

void StmtCodeGen::Visit(const hir::WhileStatement &stmt) {
    auto id = ctx_.label_id_generator().GenNewId();

    ctx_.printer().PrintLn("L.START.{}", id);

    ExprCodeGen cond_gen(ctx_);
    stmt.cond()->Accept(cond_gen);
    if (!cond_gen) return;
    ctx_.printer().PrintLn("  test %ax, %ax");
    ctx_.printer().PrintLn("  je L.END.{}", id);

    StmtCodeGen body_gen(ctx_);
    stmt.body()->Accept(body_gen);
    if (!body_gen) return;
    ctx_.printer().PrintLn("  jmp L.START.{}", id);

    ctx_.printer().PrintLn("L.END.{}", id);
}

void StmtCodeGen::Visit(const hir::IfStatement &stmt) {
    auto id = ctx_.label_id_generator().GenNewId();

    ExprCodeGen cond_gen(ctx_);
    stmt.cond()->Accept(cond_gen);
    if (!cond_gen) return;

    ctx_.printer().PrintLn("  test %ax, %ax");
    ctx_.printer().PrintLn("  je L.ELSE.{}", id);

    StmtCodeGen then_gen(ctx_);
    stmt.then_body()->Accept(then_gen);
    if (!then_gen) return;
    ctx_.printer().PrintLn("  jmp L.END.{}:", id);

    ctx_.printer().PrintLn("L.ELSE.{}:", id);

    if (stmt.else_body()) {
        StmtCodeGen else_gen(ctx_);
        stmt.else_body().value()->Accept(else_gen);
        if (!else_gen) return;
    }

    ctx_.printer().PrintLn("L.END.{}:", id);
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
