#include "expr.h"

#include "fmt/format.h"

namespace mini {

namespace hir {

std::string UnaryExpression::Op::to_string() const {
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
        fatal_error("unreachable");
        return "";
    }
}

void UnaryExpression::print(PrintableContext& ctx) const {
    ctx.printer().print(fmt::format("({}", op_.to_string()));
    expr_->print(ctx);
    ctx.printer().print(")");
}

std::string InfixExpression::Op::to_string() const {
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
        fatal_error("unreachable");
        return "";
    }
}

void InfixExpression::print(PrintableContext& ctx) const {
    ctx.printer().print("(");
    lhs_->print(ctx);
    ctx.printer().print(fmt::format(" {} ", op_.to_string()));
    rhs_->print(ctx);
    ctx.printer().print(")");
}

void IndexExpression::print(PrintableContext& ctx) const {
    expr_->print(ctx);
    ctx.printer().print("[");
    index_->print(ctx);
    ctx.printer().print("]");
}

void CallExpression::print(PrintableContext& ctx) const {
    func_->print(ctx);
    ctx.printer().print("(");
    if (!args_.empty()) {
        args_.front()->print(ctx);
        for (size_t i = 1; i < args_.size(); i++) {
            ctx.printer().print(", ");
            args_.at(i)->print(ctx);
        }
    }
    ctx.printer().print(")");
}

void AccessExpression::print(PrintableContext& ctx) const {
    expr_->print(ctx);
    ctx.printer().print(fmt::format(".{}", field_.value()));
}

void CastExpression::print(PrintableContext& ctx) const {
    expr_->print(ctx);
    ctx.printer().print(" as ");
    cast_type_->print(ctx);
}

void ESizeofExpression::print(PrintableContext& ctx) const {
    ctx.printer().print("esizeof ");
    expr_->print(ctx);
}

void TSizeofExpression::print(PrintableContext& ctx) const {
    ctx.printer().print("tsizeof ");
    type_->print(ctx);
}

void StructExpression::print(PrintableContext& ctx) const {
    if (!inits_.empty()) {
        ctx.printer().shiftr();
        ctx.printer().println(fmt::format("{} {{", name_.value()));
        for (size_t i = 0; i < inits_.size(); i++) {
            auto& init = inits_.at(i);
            ctx.printer().print(fmt::format("{}: ", init.name().value()));
            init.value()->print(ctx);
            ctx.printer().print(",");
            if (i == inits_.size() - 1) {
                ctx.printer().shiftl();
            }
            ctx.printer().println("");
        }
        ctx.printer().print("}");
    } else {
        ctx.printer().print("{}");
    }
}

void ArrayExpression::print(PrintableContext& ctx) const {
    if (!inits_.empty()) {
        ctx.printer().shiftr();
        ctx.printer().println("{");
        for (size_t i = 0; i < inits_.size(); i++) {
            auto& init = inits_.at(i);
            init->print(ctx);
            ctx.printer().print(",");
            if (i == inits_.size() - 1) {
                ctx.printer().shiftl();
            }
            ctx.printer().println("");
        }
        ctx.printer().print("}");
    } else {
        ctx.printer().print("{}");
    }
}

}  // namespace hir

}  // namespace mini
