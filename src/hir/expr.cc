#include "expr.h"

namespace mini {

namespace hir {

std::string UnaryExpression::Op::ToString() const {
    if (kind_ == Op::Ref) {
        return "&";
    } else if (kind_ == Op::Deref) {
        return "*";
    } else if (kind_ == Op::Minus) {
        return "-";
    } else if (kind_ == Op::Inv) {
        return "~";
    } else if (kind_ == Op::Neg) {
        return "!";
    } else {
        FatalError("unreachable");
        return "";
    }
}

void UnaryExpression::Print(PrintableContext& ctx) const {
    ctx.printer().Print("({}", op_.ToString());
    expr_->Print(ctx);
    ctx.printer().Print(")");
}

std::string InfixExpression::Op::ToString() const {
    if (kind_ == Op::Add) {
        return "+";
    } else if (kind_ == Op::Sub) {
        return "-";
    } else if (kind_ == Op::Mul) {
        return "*";
    } else if (kind_ == Op::Div) {
        return "/";
    } else if (kind_ == Op::Mod) {
        return "%";
    } else if (kind_ == Op::Or) {
        return "||";
    } else if (kind_ == Op::And) {
        return "&&";
    } else if (kind_ == Op::BitOr) {
        return "|";
    } else if (kind_ == Op::BitAnd) {
        return "&";
    } else if (kind_ == Op::BitXor) {
        return "^";
    } else if (kind_ == Op::Assign) {
        return "=";
    } else if (kind_ == Op::EQ) {
        return "==";
    } else if (kind_ == Op::NE) {
        return "!=";
    } else if (kind_ == Op::LT) {
        return "<";
    } else if (kind_ == Op::LE) {
        return "<=";
    } else if (kind_ == Op::GT) {
        return ">";
    } else if (kind_ == Op::GE) {
        return ">=";
    } else if (kind_ == Op::LShift) {
        return "<<";
    } else if (kind_ == Op::RShift) {
        return ">>";
    } else {
        FatalError("unreachable");
        return "";
    }
}

void InfixExpression::Print(PrintableContext& ctx) const {
    ctx.printer().Print("(");
    lhs_->Print(ctx);
    ctx.printer().Print(" {} ", op_.ToString());
    rhs_->Print(ctx);
    ctx.printer().Print(")");
}

void IndexExpression::Print(PrintableContext& ctx) const {
    expr_->Print(ctx);
    ctx.printer().Print("[");
    index_->Print(ctx);
    ctx.printer().Print("]");
}

void CallExpression::Print(PrintableContext& ctx) const {
    func_->Print(ctx);
    ctx.printer().Print("(");
    if (!args_.empty()) {
        args_.front()->Print(ctx);
        for (size_t i = 1; i < args_.size(); i++) {
            ctx.printer().Print(", ");
            args_.at(i)->Print(ctx);
        }
    }
    ctx.printer().Print(")");
}

void AccessExpression::Print(PrintableContext& ctx) const {
    expr_->Print(ctx);
    ctx.printer().Print(".{}", field_.value());
}

void CastExpression::Print(PrintableContext& ctx) const {
    expr_->Print(ctx);
    ctx.printer().Print(" as ");
    cast_type_->Print(ctx);
}

void ESizeofExpression::Print(PrintableContext& ctx) const {
    ctx.printer().Print("esizeof ");
    expr_->Print(ctx);
}

void TSizeofExpression::Print(PrintableContext& ctx) const {
    ctx.printer().Print("tsizeof ");
    type_->Print(ctx);
}

void StructExpression::Print(PrintableContext& ctx) const {
    if (!inits_.empty()) {
        ctx.printer().ShiftR();
        ctx.printer().PrintLn("{} {{", name_.value());
        for (size_t i = 0; i < inits_.size(); i++) {
            auto& init = inits_.at(i);
            ctx.printer().Print("{}: ", init.name().value());
            init.value()->Print(ctx);
            ctx.printer().Print(",");
            if (i == inits_.size() - 1) {
                ctx.printer().ShiftL();
            }
            ctx.printer().PrintLn("");
        }
        ctx.printer().Print("}");
    } else {
        ctx.printer().Print("{}");
    }
}

void ArrayExpression::Print(PrintableContext& ctx) const {
    if (!inits_.empty()) {
        ctx.printer().ShiftR();
        ctx.printer().PrintLn("{");
        for (size_t i = 0; i < inits_.size(); i++) {
            auto& init = inits_.at(i);
            init->Print(ctx);
            ctx.printer().Print(",");
            if (i == inits_.size() - 1) {
                ctx.printer().ShiftL();
            }
            ctx.printer().PrintLn("");
        }
        ctx.printer().Print("}");
    } else {
        ctx.printer().Print("{}");
    }
}

}  // namespace hir

}  // namespace mini
