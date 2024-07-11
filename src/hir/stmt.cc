#include "stmt.h"

namespace mini {

namespace hir {

void ExpressionStatement::print(PrintableContext &ctx) const {
    expr_->print(ctx);
    ctx.printer().print(";");
}

void ReturnStatement::print(PrintableContext &ctx) const {
    ctx.printer().print("return");
    if (ret_value_) {
        ctx.printer().print(" ");
        ret_value_.value()->print(ctx);
    }
    ctx.printer().print(";");
}

void WhileStatement::print(PrintableContext &ctx) const {
    ctx.printer().print("while (");
    cond_->print(ctx);
    ctx.printer().print(") ");
    body_->print(ctx);
}

void IfStatement::print(PrintableContext &ctx) const {
    ctx.printer().print("if (");
    cond_->print(ctx);
    ctx.printer().print(") ");
    then_body_->print(ctx);
    if (else_body_) {
        ctx.printer().print(" else ");
        else_body_.value()->print(ctx);
    }
}

void BlockStatement::print(PrintableContext &ctx) const {
    ctx.printer().shiftr();
    ctx.printer().println("{");
    for (size_t i = 0; i < stmts_.size(); i++) {
        stmts_.at(i)->print(ctx);
        if (i == stmts_.size() - 1) {
            ctx.printer().shiftl();
        }
        ctx.printer().println("");
    }
    ctx.printer().print("}");
}

}  // namespace hir

}  // namespace mini
