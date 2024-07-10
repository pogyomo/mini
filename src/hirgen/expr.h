#ifndef MINI_HIRGEN_EXPR_H_
#define MINI_HIRGEN_EXPR_H_

#include <memory>
#include <string>

#include "../ast/expr.h"
#include "../hir/expr.h"
#include "context.h"
#include "type.h"

namespace mini {

class ExprHirGen : public ast::ExpressionVisitor {
public:
    ExprHirGen(HirGenContext &ctx) : success_(false), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    std::unique_ptr<hir::Expression> &expr() { return expr_; }
    void visit(const ast::UnaryExpression &expr) override {
        ExprHirGen expr_gen(ctx_);
        expr.expr()->accept(expr_gen);
        if (!expr_gen) return;

        hir::UnaryExpression::Op::Kind kind;
        if (expr.op().kind() == ast::UnaryExpression::Op::Ref)
            kind = hir::UnaryExpression::Op::Ref;
        else if (expr.op().kind() == ast::UnaryExpression::Op::Deref)
            kind = hir::UnaryExpression::Op::Deref;
        else if (expr.op().kind() == ast::UnaryExpression::Op::Minus)
            kind = hir::UnaryExpression::Op::Minus;
        else if (expr.op().kind() == ast::UnaryExpression::Op::Inv)
            kind = hir::UnaryExpression::Op::Inv;
        else if (expr.op().kind() == ast::UnaryExpression::Op::Neg)
            kind = hir::UnaryExpression::Op::Neg;
        else
            fatal_error("unreachable");

        hir::UnaryExpression::Op op(kind, expr.op().span());
        expr_ = std::make_unique<hir::UnaryExpression>(
            op, std::move(expr_gen.expr_), expr.span());
        success_ = true;
    }
    void visit(const ast::InfixExpression &expr) override {
        ExprHirGen lhs_gen(ctx_);
        ExprHirGen rhs_gen(ctx_);
        expr.lhs()->accept(lhs_gen);
        expr.rhs()->accept(rhs_gen);
        if (!lhs_gen || !rhs_gen) return;

        hir::InfixExpression::Op::Kind kind;
        if (expr.op().kind() == ast::InfixExpression::Op::Add)
            kind = hir::InfixExpression::Op::Add;
        else if (expr.op().kind() == ast::InfixExpression::Op::Sub)
            kind = hir::InfixExpression::Op::Sub;
        else if (expr.op().kind() == ast::InfixExpression::Op::Mul)
            kind = hir::InfixExpression::Op::Mul;
        else if (expr.op().kind() == ast::InfixExpression::Op::Div)
            kind = hir::InfixExpression::Op::Div;
        else if (expr.op().kind() == ast::InfixExpression::Op::Mod)
            kind = hir::InfixExpression::Op::Mod;
        else if (expr.op().kind() == ast::InfixExpression::Op::Or)
            kind = hir::InfixExpression::Op::Or;
        else if (expr.op().kind() == ast::InfixExpression::Op::And)
            kind = hir::InfixExpression::Op::And;
        else if (expr.op().kind() == ast::InfixExpression::Op::BitOr)
            kind = hir::InfixExpression::Op::BitOr;
        else if (expr.op().kind() == ast::InfixExpression::Op::BitAnd)
            kind = hir::InfixExpression::Op::BitAnd;
        else if (expr.op().kind() == ast::InfixExpression::Op::BitXor)
            kind = hir::InfixExpression::Op::BitXor;
        else if (expr.op().kind() == ast::InfixExpression::Op::Assign)
            kind = hir::InfixExpression::Op::Assign;
        else if (expr.op().kind() == ast::InfixExpression::Op::EQ)
            kind = hir::InfixExpression::Op::EQ;
        else if (expr.op().kind() == ast::InfixExpression::Op::NE)
            kind = hir::InfixExpression::Op::NE;
        else if (expr.op().kind() == ast::InfixExpression::Op::LT)
            kind = hir::InfixExpression::Op::LT;
        else if (expr.op().kind() == ast::InfixExpression::Op::LE)
            kind = hir::InfixExpression::Op::LE;
        else if (expr.op().kind() == ast::InfixExpression::Op::GT)
            kind = hir::InfixExpression::Op::GT;
        else if (expr.op().kind() == ast::InfixExpression::Op::GE)
            kind = hir::InfixExpression::Op::GE;
        else if (expr.op().kind() == ast::InfixExpression::Op::LShift)
            kind = hir::InfixExpression::Op::LShift;
        else if (expr.op().kind() == ast::InfixExpression::Op::RShift)
            kind = hir::InfixExpression::Op::RShift;
        else
            fatal_error("unreachable");

        hir::InfixExpression::Op op(kind, expr.op().span());
        expr_ = std::make_unique<hir::InfixExpression>(
            std::move(lhs_gen.expr_), op, std::move(rhs_gen.expr_),
            expr.span());
        success_ = true;
    }
    void visit(const ast::IndexExpression &expr) override {
        ExprHirGen expr_gen(ctx_);
        expr.expr()->accept(expr_gen);
        if (!expr_gen) return;

        ExprHirGen index_gen(ctx_);
        expr.index()->accept(index_gen);
        if (!index_gen) return;

        expr_ = std::make_unique<hir::IndexExpression>(
            std::move(expr_gen.expr_), std::move(index_gen.expr_), expr.span());
        success_ = true;
    }
    void visit(const ast::CallExpression &expr) override {
        ExprHirGen func_gen(ctx_);
        expr.func()->accept(func_gen);
        if (!func_gen) return;

        std::vector<std::unique_ptr<hir::Expression>> args;
        for (const auto &arg : expr.args()) {
            ExprHirGen gen(ctx_);
            arg->accept(gen);
            if (!gen) return;

            args.emplace_back(std::move(gen.expr_));
        }

        expr_ = std::make_unique<hir::CallExpression>(
            std::move(func_gen.expr_), std::move(args), expr.span());
        success_ = true;
    }
    void visit(const ast::AccessExpression &expr) override {
        ExprHirGen gen(ctx_);
        expr.expr()->accept(gen);
        if (!gen) return;

        hir::AccessExpressionField field(std::string(expr.field().name()),
                                         expr.field().span());
        expr_ = std::make_unique<hir::AccessExpression>(
            std::move(gen.expr_), std::move(field), expr.span());
        success_ = true;
    }
    void visit(const ast::CastExpression &expr) override {
        ExprHirGen expr_gen(ctx_);
        expr.expr()->accept(expr_gen);
        if (!expr_gen) return;

        TypeHirGen type_gen(ctx_);
        expr.type()->accept(type_gen);
        if (!type_gen) return;

        expr_ = std::make_unique<hir::CastExpression>(
            std::move(expr_gen.expr_), std::move(type_gen.type()), expr.span());
        success_ = true;
    }
    void visit(const ast::ESizeofExpression &expr) override {
        ExprHirGen gen(ctx_);
        expr.expr()->accept(gen);
        if (!gen) return;

        expr_ = std::make_unique<hir::ESizeofExpression>(std::move(gen.expr_),
                                                         expr.span());
        success_ = true;
    }
    void visit(const ast::TSizeofExpression &expr) override {
        TypeHirGen gen(ctx_);
        expr.type()->accept(gen);
        if (!gen) return;

        expr_ = std::make_unique<hir::TSizeofExpression>(std::move(gen.type()),
                                                         expr.span());
        success_ = true;
    }
    void visit(const ast::EnumSelectExpression &expr) override {
        hir::EnumSelectExpressionSrc src(std::string(expr.src().name()),
                                         expr.src().span());
        hir::EnumSelectExpressionDst dst(std::string(expr.dst().name()),
                                         expr.dst().span());

        expr_ = std::make_unique<hir::EnumSelectExpression>(
            std::move(src), std::move(dst), expr.span());
        success_ = true;
    }
    void visit(const ast::VariableExpression &expr) override {
        if (!ctx_.translator().translatable(expr.value())) {
            ReportInfo info(expr.span(), "no such variable exists", "");
            report(ctx_.ctx(), ReportLevel::Error, info);
            return;
        }

        std::string translated = ctx_.translator().translate(expr.value());
        expr_ = std::make_unique<hir::VariableExpression>(std::move(translated),
                                                          expr.span());
        success_ = true;
    }
    void visit(const ast::IntegerExpression &expr) override {
        expr_ =
            std::make_unique<hir::IntegerExpression>(expr.value(), expr.span());
        success_ = true;
    }
    void visit(const ast::StringExpression &expr) override {
        expr_ = std::make_unique<hir::StringExpression>(
            std::string(expr.value()), expr.span());
        success_ = true;
    }
    void visit(const ast::CharExpression &expr) override {
        expr_ =
            std::make_unique<hir::CharExpression>(expr.value(), expr.span());
        success_ = true;
    }
    void visit(const ast::BoolExpression &expr) override {
        expr_ =
            std::make_unique<hir::BoolExpression>(expr.value(), expr.span());
        success_ = true;
    }
    void visit(const ast::StructExpression &expr) override {
        std::vector<hir::StructExpressionInit> inits;
        for (const auto &init : expr.inits()) {
            ExprHirGen gen(ctx_);
            init.value()->accept(gen);
            if (!gen) return;

            hir::StructExpressionInitName name(std::string(init.name().name()),
                                               init.name().span());
            inits.emplace_back(std::move(name), std::move(gen.expr_));
        }

        hir::StructExpressionName name(std::string(expr.name().name()),
                                       expr.name().span());
        expr_ = std::make_unique<hir::StructExpression>(
            std::move(name), std::move(inits), expr.span());
        success_ = true;
    }
    void visit(const ast::ArrayExpression &expr) override {
        std::vector<std::unique_ptr<hir::Expression>> inits;
        for (const auto &init : expr.inits()) {
            ExprHirGen gen(ctx_);
            init->accept(gen);
            if (!gen) return;

            inits.emplace_back(std::move(gen.expr_));
        }
        expr_ = std::make_unique<hir::ArrayExpression>(std::move(inits),
                                                       expr.span());
        success_ = true;
    }

private:
    bool success_;
    std::unique_ptr<hir::Expression> expr_;
    HirGenContext &ctx_;
};

}  // namespace mini

#endif  // MINI_HIRGEN_EXPR_H_
