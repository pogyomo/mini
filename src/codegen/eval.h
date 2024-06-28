#ifndef MINI_CODEGEN_EVAL_H_
#define MINI_CODEGEN_EVAL_H_

#include "../ast/expr.h"
#include "../context.h"
#include "../report.h"

class ExprConstEval : public ExpressionVisitor {
public:
    ExprConstEval(Context &ctx, std::ostream &os)
        : success_(true), value_(0), ctx_(ctx), os_(os) {}
    bool success() const { return success_; }
    uint64_t value() const { return value_; }
    void visit(const UnaryExpression &expr) override {
        ExprConstEval value_eval(ctx_, os_);
        expr.expr()->accept(value_eval);
        if (!(success_ &= value_eval.success_)) return;
        auto value = value_eval.value();

        if (expr.op().kind() == UnaryExpression::Op::Inv) {
            value_ = ~value;
        } else {
            ReportInfo info(
                expr.op().span(),
                "only some arthmetic operator is available on const expression",
                "");
            report(ctx_, ReportLevel::Error, info);
        }
    }
    void visit(const InfixExpression &expr) override {
        ExprConstEval lhs_eval(ctx_, os_);
        expr.lhs()->accept(lhs_eval);
        if (!(success_ &= lhs_eval.success_)) return;
        auto lhs = lhs_eval.value();

        ExprConstEval rhs_eval(ctx_, os_);
        expr.rhs()->accept(rhs_eval);
        if (!(success_ &= rhs_eval.success_)) return;
        auto rhs = rhs_eval.value();

        if (expr.op().kind() == InfixExpression::Op::Add) {
            value_ = lhs + rhs;
        } else if (expr.op().kind() == InfixExpression::Op::Sub) {
            value_ = lhs - rhs;
        } else if (expr.op().kind() == InfixExpression::Op::Mul) {
            value_ = lhs * rhs;
        } else if (expr.op().kind() == InfixExpression::Op::Div) {
            value_ = lhs / rhs;
        } else if (expr.op().kind() == InfixExpression::Op::Mod) {
            value_ = lhs % rhs;
        } else if (expr.op().kind() == InfixExpression::Op::BitAnd) {
            value_ = lhs & rhs;
        } else if (expr.op().kind() == InfixExpression::Op::BitOr) {
            value_ = lhs | rhs;
        } else if (expr.op().kind() == InfixExpression::Op::BitXor) {
            value_ = lhs ^ rhs;
        } else if (expr.op().kind() == InfixExpression::Op::LShift) {
            value_ = lhs << rhs;
        } else if (expr.op().kind() == InfixExpression::Op::RShift) {
            value_ = lhs >> rhs;
        } else {
            ReportInfo info(
                expr.op().span(),
                "only some arthmetic operator is available on const expression",
                "");
            report(ctx_, ReportLevel::Error, info);
        }
    }
    void visit(const IndexExpression &expr) override {
        ReportInfo info(expr.span(),
                        "indexing is not expected on const expression", "");
        report(ctx_, ReportLevel::Error, info);
    }
    void visit(const CallExpression &expr) override {
        ReportInfo info(expr.span(),
                        "function call is not expected on const expression",
                        "");
        report(ctx_, ReportLevel::Error, info);
    }
    void visit(const AccessExpression &expr) override {
        ReportInfo info(expr.span(),
                        "access is not expected on const expression", "");
        report(ctx_, ReportLevel::Error, info);
    }
    void visit(const CastExpression &expr) override {
        ReportInfo info(expr.span(), "cast is not expected on const expression",
                        "");
        report(ctx_, ReportLevel::Error, info);
    }
    void visit(const ESizeofExpression &expr) override {
        ReportInfo info(expr.span(),
                        "esizeof is not expected on const expression", "");
        report(ctx_, ReportLevel::Error, info);
    }
    void visit(const TSizeofExpression &expr) override {
        ReportInfo info(expr.span(),
                        "tsizeof is not expected on const expression", "");
        report(ctx_, ReportLevel::Error, info);
    }
    void visit(const EnumSelectExpression &expr) override {
        ReportInfo info(expr.span(),
                        "enum selector is not expected on const expression",
                        "");
        report(ctx_, ReportLevel::Error, info);
    }
    void visit(const VariableExpression &expr) override {
        ReportInfo info(expr.span(),
                        "variable is not expected on const expression", "");
        report(ctx_, ReportLevel::Error, info);
    }
    void visit(const IntegerExpression &expr) override {
        value_ = expr.value();
    }
    void visit(const StringExpression &expr) override {
        ReportInfo info(expr.span(),
                        "string literal should not on const expression", "");
        report(ctx_, ReportLevel::Error, info);
    }
    void visit(const BoolExpression &expr) override {
        ReportInfo info(expr.span(),
                        "boolean literal should not on const expression", "");
        report(ctx_, ReportLevel::Error, info);
    }

private:
    bool success_;
    uint64_t value_;
    Context &ctx_;
    std::ostream &os_;
};

#endif  // MINI_CODEGEN_EVAL_H_
