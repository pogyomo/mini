#include "stmt.h"

#include <utility>

#include "expr.h"
#include "item.h"

namespace mini {

void StmtHirGen::Visit(const ast::ExpressionStatement &stmt) {
    ExprHirGen gen(ctx_);
    stmt.expr()->Accept(gen);
    if (!gen) return;

    stmt_ = std::make_unique<hir::ExpressionStatement>(std::move(gen.expr()),
                                                       stmt.span());
    success_ = true;
}

void StmtHirGen::Visit(const ast::ReturnStatement &stmt) {
    std::optional<std::unique_ptr<hir::Expression>> ret_value;
    if (stmt.expr()) {
        ExprHirGen gen(ctx_);
        stmt.expr().value()->Accept(gen);
        if (!gen) return;
        ret_value.emplace(std::move(gen.expr()));
    }

    stmt_ = std::make_unique<hir::ReturnStatement>(std::move(ret_value),
                                                   stmt.span());
    success_ = true;
}

void StmtHirGen::Visit(const ast::BreakStatement &stmt) {
    stmt_ = std::make_unique<hir::BreakStatement>(stmt.span());
    success_ = true;
}

void StmtHirGen::Visit(const ast::ContinueStatement &stmt) {
    stmt_ = std::make_unique<hir::ContinueStatement>(stmt.span());
    success_ = true;
}

void StmtHirGen::Visit(const ast::WhileStatement &stmt) {
    ExprHirGen cond_gen(ctx_);
    stmt.cond()->Accept(cond_gen);
    if (!cond_gen) return;

    StmtHirGen body_gen(ctx_);
    stmt.body()->Accept(body_gen);
    if (!body_gen) return;

    stmt_ = std::make_unique<hir::WhileStatement>(
        std::move(cond_gen.expr()), std::move(body_gen.stmt_), stmt.span());
    success_ = true;
}

void StmtHirGen::Visit(const ast::IfStatement &stmt) {
    ExprHirGen cond_gen(ctx_);
    stmt.cond()->Accept(cond_gen);
    if (!cond_gen) return;

    StmtHirGen then_gen(ctx_);
    stmt.body()->Accept(then_gen);
    if (!then_gen) return;

    std::optional<std::unique_ptr<hir::Statement>> else_body;
    if (stmt.else_clause()) {
        StmtHirGen else_gen(ctx_);
        stmt.else_clause()->body()->Accept(else_gen);
        if (!else_gen) return;
        else_body.emplace(std::move(else_gen.stmt_));
    }

    stmt_ = std::make_unique<hir::IfStatement>(
        std::move(cond_gen.expr()), std::move(then_gen.stmt_),
        std::move(else_body), stmt.span());
    success_ = true;
}

void StmtHirGen::Visit(const ast::BlockStatement &stmt) {
    std::vector<std::unique_ptr<hir::Statement>> stmts;
    std::vector<hir::VariableDeclaration> decls;

    ctx_.translator().EnterScope();
    for (const auto &item : stmt.items()) {
        if (!hirgen_block_item(ctx_, item, stmts, decls)) return;
    }
    ctx_.translator().LeaveScope();

    decls_ = decls;
    stmt_ =
        std::make_unique<hir::BlockStatement>(std::move(stmts), stmt.span());
    success_ = true;
}

}  // namespace mini
