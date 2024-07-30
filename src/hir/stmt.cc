#include "stmt.h"

namespace mini {

namespace hir {

void ExpressionStatement::Print(PrintableContext &ctx) const {
    expr_->Print(ctx);
    ctx.printer().Print(";");
}

void ReturnStatement::Print(PrintableContext &ctx) const {
    ctx.printer().Print("return");
    if (ret_value_) {
        ctx.printer().Print(" ");
        ret_value_.value()->Print(ctx);
    }
    ctx.printer().Print(";");
}

void WhileStatement::Print(PrintableContext &ctx) const {
    ctx.printer().Print("while (");
    cond_->Print(ctx);
    ctx.printer().Print(") ");
    body_->Print(ctx);
}

void IfStatement::Print(PrintableContext &ctx) const {
    ctx.printer().Print("if (");
    cond_->Print(ctx);
    ctx.printer().Print(") ");
    then_body_->Print(ctx);
    if (else_body_) {
        ctx.printer().Print(" else ");
        else_body_.value()->Print(ctx);
    }
}

void BlockStatement::Print(PrintableContext &ctx) const {
    if (!stmts_.empty()) {
        ctx.printer().ShiftR();
        ctx.printer().PrintLn("{{");
        for (size_t i = 0; i < stmts_.size(); i++) {
            stmts_.at(i)->Print(ctx);
            if (i == stmts_.size() - 1) {
                ctx.printer().ShiftL();
            }
            ctx.printer().PrintLn("");
        }
        ctx.printer().Print("}}");
    } else {
        ctx.printer().PrintLn("{{");
        ctx.printer().Print("}}");
    }
}

}  // namespace hir

}  // namespace mini
