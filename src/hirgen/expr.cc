#include "expr.h"

#include "../report.h"
#include "type.h"

namespace mini {

void ExprHirGen::Visit(const ast::UnaryExpression &expr) {
    ExprHirGen expr_gen(ctx_);
    expr.expr()->Accept(expr_gen);
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
        FatalError("unreachable");

    hir::UnaryExpression::Op op(kind, expr.op().span());
    expr_ = std::make_unique<hir::UnaryExpression>(
        op, std::move(expr_gen.expr_), expr.span());
    success_ = true;
}

void ExprHirGen::Visit(const ast::InfixExpression &expr) {
    ExprHirGen lhs_gen(ctx_);
    ExprHirGen rhs_gen(ctx_);
    expr.lhs()->Accept(lhs_gen);
    expr.rhs()->Accept(rhs_gen);
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
        FatalError("unreachable");

    hir::InfixExpression::Op op(kind, expr.op().span());
    expr_ = std::make_unique<hir::InfixExpression>(
        std::move(lhs_gen.expr_), op, std::move(rhs_gen.expr_), expr.span());
    success_ = true;
}

void ExprHirGen::Visit(const ast::IndexExpression &expr) {
    ExprHirGen expr_gen(ctx_);
    expr.expr()->Accept(expr_gen);
    if (!expr_gen) return;

    ExprHirGen index_gen(ctx_);
    expr.index()->Accept(index_gen);
    if (!index_gen) return;

    expr_ = std::make_unique<hir::IndexExpression>(
        std::move(expr_gen.expr_), std::move(index_gen.expr_), expr.span());
    success_ = true;
}
void ExprHirGen::Visit(const ast::CallExpression &expr) {
    ExprHirGen func_gen(ctx_);
    expr.func()->Accept(func_gen);
    if (!func_gen) return;

    std::vector<std::unique_ptr<hir::Expression>> args;
    for (const auto &arg : expr.args()) {
        ExprHirGen gen(ctx_);
        arg->Accept(gen);
        if (!gen) return;

        args.emplace_back(std::move(gen.expr_));
    }

    expr_ = std::make_unique<hir::CallExpression>(std::move(func_gen.expr_),
                                                  std::move(args), expr.span());
    success_ = true;
}

void ExprHirGen::Visit(const ast::AccessExpression &expr) {
    ExprHirGen gen(ctx_);
    expr.expr()->Accept(gen);
    if (!gen) return;

    hir::AccessExpressionField field(std::string(expr.field().name()),
                                     expr.field().span());
    expr_ = std::make_unique<hir::AccessExpression>(
        std::move(gen.expr_), std::move(field), expr.span());
    success_ = true;
}

void ExprHirGen::Visit(const ast::CastExpression &expr) {
    ExprHirGen expr_gen(ctx_);
    expr.expr()->Accept(expr_gen);
    if (!expr_gen) return;

    TypeHirGen type_gen(ctx_);
    expr.type()->Accept(type_gen);
    if (!type_gen) return;

    expr_ = std::make_unique<hir::CastExpression>(
        std::move(expr_gen.expr_), std::move(type_gen.type()), expr.span());
    success_ = true;
}

void ExprHirGen::Visit(const ast::ESizeofExpression &expr) {
    ExprHirGen gen(ctx_);
    expr.expr()->Accept(gen);
    if (!gen) return;

    expr_ = std::make_unique<hir::ESizeofExpression>(std::move(gen.expr_),
                                                     expr.span());
    success_ = true;
}

void ExprHirGen::Visit(const ast::TSizeofExpression &expr) {
    TypeHirGen gen(ctx_);
    expr.type()->Accept(gen);
    if (!gen) return;

    expr_ = std::make_unique<hir::TSizeofExpression>(std::move(gen.type()),
                                                     expr.span());
    success_ = true;
}

void ExprHirGen::Visit(const ast::EnumSelectExpression &expr) {
    hir::EnumSelectExpressionSrc src(std::string(expr.src().name()),
                                     expr.src().span());
    hir::EnumSelectExpressionDst dst(std::string(expr.dst().name()),
                                     expr.dst().span());

    expr_ = std::make_unique<hir::EnumSelectExpression>(
        std::move(src), std::move(dst), expr.span());
    success_ = true;
}

void ExprHirGen::Visit(const ast::VariableExpression &expr) {
    if (!ctx_.translator().Translatable(expr.value())) {
        ReportInfo info(expr.span(), "no such name exists", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    std::string translated = ctx_.translator().Translate(expr.value());
    expr_ = std::make_unique<hir::VariableExpression>(std::move(translated),
                                                      expr.span());
    success_ = true;
}

void ExprHirGen::Visit(const ast::IntegerExpression &expr) {
    expr_ = std::make_unique<hir::IntegerExpression>(expr.value(), expr.span());
    success_ = true;
}

void ExprHirGen::Visit(const ast::StringExpression &expr) {
    expr_ = std::make_unique<hir::StringExpression>(std::string(expr.value()),
                                                    expr.span());
    ctx_.string_table().AddString(std::string(expr.value()));
    success_ = true;
}

void ExprHirGen::Visit(const ast::CharExpression &expr) {
    expr_ = std::make_unique<hir::CharExpression>(expr.value(), expr.span());
    success_ = true;
}

void ExprHirGen::Visit(const ast::BoolExpression &expr) {
    expr_ = std::make_unique<hir::BoolExpression>(expr.value(), expr.span());
    success_ = true;
}

void ExprHirGen::Visit(const ast::StructExpression &expr) {
    std::vector<hir::StructExpressionInit> inits;
    for (const auto &init : expr.inits()) {
        ExprHirGen gen(ctx_);
        init.value()->Accept(gen);
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

void ExprHirGen::Visit(const ast::ArrayExpression &expr) {
    std::vector<std::unique_ptr<hir::Expression>> inits;
    for (const auto &init : expr.inits()) {
        ExprHirGen gen(ctx_);
        init->Accept(gen);
        if (!gen) return;

        inits.emplace_back(std::move(gen.expr_));
    }
    expr_ =
        std::make_unique<hir::ArrayExpression>(std::move(inits), expr.span());
    success_ = true;
}

}  // namespace mini
