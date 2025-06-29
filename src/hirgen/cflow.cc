#include "cflow.h"

#include "../context.h"
#include "../hir/decl.h"
#include "../hir/stmt.h"
#include "../hir/type.h"
#include "../report.h"

namespace mini {

class CompleteReturnChecker : public hir::StatementVisitor {
public:
    CompleteReturnChecker() : success_(false) {}
    explicit operator bool() const { return success_; }
    void Visit(const hir::ExpressionStatement &) override {}
    void Visit(const hir::ReturnStatement &) override { success_ = true; }
    void Visit(const hir::BreakStatement &) override {}
    void Visit(const hir::ContinueStatement &) override {}
    void Visit(const hir::WhileStatement &) override {}
    void Visit(const hir::IfStatement &stmt) override {
        if (!stmt.else_body()) return;
        CompleteReturnChecker then_check;
        CompleteReturnChecker else_check;
        stmt.then_body()->Accept(then_check);
        stmt.else_body().value()->Accept(else_check);
        success_ = then_check.success_ && else_check.success_;
    }
    void Visit(const hir::BlockStatement &stmt) override {
        for (auto &stmt_ : stmt.stmts()) {
            CompleteReturnChecker check;
            stmt_->Accept(check);
            if (check.success_) success_ = true;
        }
    }

private:
    bool success_;
};

static bool ControlFlowCheck(Context &ctx,
                             const hir::FunctionDeclaration &func) {
    auto ret = func.ret();
    if (!func.body() || (ret->IsBuiltin() &&
                         ret->ToBuiltin()->kind() == hir::BuiltinType::Void)) {
        return true;
    }

    // Check this function always reach to return.
    bool always_return = false;
    auto &body = func.body().value();
    for (auto &stmt : body.stmts()) {
        CompleteReturnChecker check;
        stmt->Accept(check);
        if (check) {
            always_return = true;
            break;
        }
    }

    if (!always_return) {
        ReportInfo info(func.span(),
                        "function doesn't return for all control flow",
                        "add return at end of block");
        Report(ctx, ReportLevel::Error, info);
        return false;
    } else {
        return true;
    }
}

void ControlFlowChecker::Visit(const hir::StructDeclaration &) {
    success_ = true;
}

void ControlFlowChecker::Visit(const hir::EnumDeclaration &) {
    success_ = true;
}

void ControlFlowChecker::Visit(const hir::FunctionDeclaration &decl) {
    success_ = ControlFlowCheck(ctx_, decl);
}

}  // namespace mini
