#ifndef MINI_HIRGEN_STMT_H_
#define MINI_HIRGEN_STMT_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../ast/stmt.h"
#include "../hir/decl.h"
#include "../hir/stmt.h"
#include "context.h"
#include "expr.h"
#include "item.h"
#include "type.h"

namespace mini {

class StmtHirGen : public ast::StatementVisitor {
public:
    StmtHirGen(HirGenContext &ctx)
        : success_(false), stmt_(nullptr), decls_(), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    std::unique_ptr<hir::Statement> &stmt() { return stmt_; }
    std::vector<hir::VariableDeclaration> &decls() { return decls_; }
    void visit(const ast::ExpressionStatement &stmt) override {
        ExprHirGen gen(ctx_);
        stmt.expr()->accept(gen);
        if (!gen) return;

        stmt_ = std::make_unique<hir::ExpressionStatement>(
            std::move(gen.expr()), stmt.span());
        success_ = true;
    }
    void visit(const ast::ReturnStatement &stmt) override {
        std::optional<std::unique_ptr<hir::Expression>> ret_value;
        if (stmt.expr()) {
            ExprHirGen gen(ctx_);
            stmt.expr().value()->accept(gen);
            if (!gen) return;
            ret_value.emplace(std::move(gen.expr()));
        }

        stmt_ = std::make_unique<hir::ReturnStatement>(std::move(ret_value),
                                                       stmt.span());
        success_ = true;
    }
    void visit(const ast::BreakStatement &stmt) override {
        stmt_ = std::make_unique<hir::BreakStatement>(stmt.span());
        success_ = true;
    }
    void visit(const ast::ContinueStatement &stmt) override {
        stmt_ = std::make_unique<hir::ContinueStatement>(stmt.span());
        success_ = true;
    }
    void visit(const ast::WhileStatement &stmt) override {
        ExprHirGen cond_gen(ctx_);
        stmt.cond()->accept(cond_gen);
        if (!cond_gen) return;

        StmtHirGen body_gen(ctx_);
        stmt.body()->accept(body_gen);
        if (!body_gen) return;

        stmt_ = std::make_unique<hir::WhileStatement>(
            std::move(cond_gen.expr()), std::move(body_gen.stmt_), stmt.span());
        success_ = true;
    }
    void visit(const ast::IfStatement &stmt) override {
        ExprHirGen cond_gen(ctx_);
        stmt.cond()->accept(cond_gen);
        if (!cond_gen) return;

        StmtHirGen then_gen(ctx_);
        stmt.body()->accept(then_gen);
        if (!then_gen) return;

        std::optional<std::unique_ptr<hir::Statement>> else_body;
        if (stmt.else_clause()) {
            StmtHirGen else_gen(ctx_);
            stmt.else_clause()->body()->accept(else_gen);
            if (!else_gen) return;
            else_body.emplace(std::move(else_gen.stmt_));
        }

        stmt_ = std::make_unique<hir::IfStatement>(
            std::move(cond_gen.expr()), std::move(then_gen.stmt_),
            std::move(else_body), stmt.span());
        success_ = true;
    }
    void visit(const ast::BlockStatement &stmt) override {
        std::vector<std::unique_ptr<hir::Statement>> stmts;
        std::vector<hir::VariableDeclaration> decls;

        ctx_.translator().enter_scope();
        for (const auto &item : stmt.items()) {
            if (!hirgen_block_item(ctx_, item, stmts, decls)) return;
        }
        ctx_.translator().leave_scope();

        decls_ = decls;
        stmt_ = std::make_unique<hir::BlockStatement>(std::move(stmts),
                                                      stmt.span());
        success_ = true;
    }

private:
    bool success_;
    std::unique_ptr<hir::Statement> stmt_;
    std::vector<hir::VariableDeclaration> decls_;
    HirGenContext &ctx_;
};

}  // namespace mini

#endif  // MINI_HIRGEN_STMT_H_
