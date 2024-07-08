#ifndef MINI_EVAL_H_
#define MINI_EVAL_H_

#include "ast/expr.h"
#include "report.h"

namespace mini {

class ConstEval : public ast::ExpressionVisitor {
public:
    ConstEval(Context &ctx) : success_(false), value_(0), ctx_(ctx) {}
    explicit operator bool() { return success_; }
    uint64_t value() const { return value_; }
    void visit(const ast::UnaryExpression &expr) override {
        ConstEval eval(ctx_);
        expr.expr()->accept(eval);
        if (!eval.success_) return;

        if (expr.op().kind() == ast::UnaryExpression::Op::Inv) {
            success_ = true;
            value_ = ~eval.value_;
        } else {
            report_error(expr.op().span(), "this unary operator");
        }
    }
    void visit(const ast::InfixExpression &expr) override {
        ConstEval lhs(ctx_);
        expr.lhs()->accept(lhs);
        if (!lhs.success_) return;

        ConstEval rhs(ctx_);
        expr.rhs()->accept(rhs);
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
    void visit(const ast::IndexExpression &expr) override {
        report_error(expr.span(), "indexing array");
    }
    void visit(const ast::CallExpression &expr) override {
        report_error(expr.span(), "calling function");
    }
    void visit(const ast::AccessExpression &expr) override {
        report_error(expr.span(), "accessing to struct");
    }
    void visit(const ast::CastExpression &expr) override {
        report_error(expr.span(), "cast");
    }
    void visit(const ast::ESizeofExpression &expr) override {
        report_error(expr.span(), "esizeof");
    }
    void visit(const ast::TSizeofExpression &expr) override {
        report_error(expr.span(), "tsizeof");
    }
    void visit(const ast::EnumSelectExpression &expr) override {
        report_error(expr.span(), "enum");
    }
    void visit(const ast::VariableExpression &expr) override {
        report_error(expr.span(), "variable");
    }
    void visit(const ast::IntegerExpression &expr) override {
        success_ = true;
        value_ = expr.value();
    }
    void visit(const ast::StringExpression &expr) override {
        report_error(expr.span(), "string");
    }
    void visit(const ast::CharExpression &expr) override {
        report_error(expr.span(), "char");
    }
    void visit(const ast::BoolExpression &expr) override {
        report_error(expr.span(), "bool");
    }
    void visit(const ast::StructExpression &expr) override {
        report_error(expr.span(), "struct initializer");
    }
    void visit(const ast::ArrayExpression &expr) override {
        report_error(expr.span(), "array initializer");
    }

private:
    void report_error(Span span, const std::string &what) {
        ReportInfo info(span, "at evaluating constant expression",
                        what + " is not allowed at constant expression");
        report(ctx_, ReportLevel::Error, info);
    }

    bool success_;
    uint64_t value_;
    Context &ctx_;
};

};  // namespace mini

#endif  // MINI_EVAL_H_
