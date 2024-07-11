#include "eval.h"

#include "report.h"

namespace mini {

void ConstEval::Visit(const ast::UnaryExpression &expr) {
    ConstEval eval(ctx_);
    expr.expr()->Accept(eval);
    if (!eval.success_) return;

    if (expr.op().kind() == ast::UnaryExpression::Op::Inv) {
        success_ = true;
        value_ = ~eval.value_;
    } else {
        report_error(expr.op().span(), "this unary operator");
    }
}

void ConstEval::Visit(const ast::InfixExpression &expr) {
    ConstEval lhs(ctx_);
    expr.lhs()->Accept(lhs);
    if (!lhs.success_) return;

    ConstEval rhs(ctx_);
    expr.rhs()->Accept(rhs);
    if (!rhs.success_) return;

    if (expr.op().kind() == ast::InfixExpression::Op::BitAnd) {
        success_ = true;
        value_ = lhs.value_ & rhs.value_;
    } else if (expr.op().kind() == ast::InfixExpression::Op::BitOr) {
        success_ = true;
        value_ = lhs.value_ | rhs.value_;
    } else if (expr.op().kind() == ast::InfixExpression::Op::BitXor) {
        success_ = true;
        value_ = lhs.value_ ^ rhs.value_;
    } else if (expr.op().kind() == ast::InfixExpression::Op::LShift) {
        success_ = true;
        value_ = lhs.value_ << rhs.value_;
    } else if (expr.op().kind() == ast::InfixExpression::Op::RShift) {
        success_ = true;
        value_ = lhs.value_ >> rhs.value_;
    } else if (expr.op().kind() == ast::InfixExpression::Op::Add) {
        success_ = true;
        value_ = lhs.value_ + rhs.value_;
    } else if (expr.op().kind() == ast::InfixExpression::Op::Sub) {
        success_ = true;
        value_ = lhs.value_ - rhs.value_;
    } else if (expr.op().kind() == ast::InfixExpression::Op::Mul) {
        success_ = true;
        value_ = lhs.value_ * rhs.value_;
    } else if (expr.op().kind() == ast::InfixExpression::Op::Div) {
        success_ = true;
        value_ = lhs.value_ / rhs.value_;
    } else if (expr.op().kind() == ast::InfixExpression::Op::Mod) {
        success_ = true;
        value_ = lhs.value_ % rhs.value_;
    } else {
        report_error(expr.op().span(), "this infix operator");
    }
}

void ConstEval::Visit(const ast::IndexExpression &expr) {
    report_error(expr.span(), "indexing array");
}

void ConstEval::Visit(const ast::CallExpression &expr) {
    report_error(expr.span(), "calling function");
}

void ConstEval::Visit(const ast::AccessExpression &expr) {
    report_error(expr.span(), "accessing to struct");
}

void ConstEval::Visit(const ast::CastExpression &expr) {
    report_error(expr.span(), "cast");
}

void ConstEval::Visit(const ast::ESizeofExpression &expr) {
    report_error(expr.span(), "esizeof");
}

void ConstEval::Visit(const ast::TSizeofExpression &expr) {
    report_error(expr.span(), "tsizeof");
}

void ConstEval::Visit(const ast::EnumSelectExpression &expr) {
    report_error(expr.span(), "enum");
}

void ConstEval::Visit(const ast::VariableExpression &expr) {
    report_error(expr.span(), "variable");
}

void ConstEval::Visit(const ast::IntegerExpression &expr) {
    success_ = true;
    value_ = expr.value();
}

void ConstEval::Visit(const ast::StringExpression &expr) {
    report_error(expr.span(), "string");
}

void ConstEval::Visit(const ast::CharExpression &expr) {
    report_error(expr.span(), "char");
}

void ConstEval::Visit(const ast::BoolExpression &expr) {
    report_error(expr.span(), "bool");
}

void ConstEval::Visit(const ast::StructExpression &expr) {
    report_error(expr.span(), "struct initializer");
}

void ConstEval::Visit(const ast::ArrayExpression &expr) {
    report_error(expr.span(), "array initializer");
}

void ConstEval::report_error(Span span, const std::string &what) {
    ReportInfo info(span, "at evaluating constant expression",
                    what + " is not allowed at constant expression");
    Report(ctx_, ReportLevel::Error, info);
}

}  // namespace mini
