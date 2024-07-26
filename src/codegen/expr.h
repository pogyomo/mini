#ifndef MINI_CODEGEN_EXPR_H_
#define MINI_CODEGEN_EXPR_H_

#include <memory>

#include "../hir/expr.h"
#include "context.h"

namespace mini {

// Evaluate expression, and store result to rax.
// If stack allocation is required, it changes CalleeSize in LVarTable, so
// caller should restore stack after calling this.
class ExprCodeGen : public hir::ExpressionVisitor {
public:
    ExprCodeGen(CodeGenContext &ctx) : success_(false), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    const std::shared_ptr<hir::Type> &inferred() const { return inferred_; }
    void Visit(const hir::UnaryExpression &expr) override;
    void Visit(const hir::InfixExpression &expr) override;
    void Visit(const hir::IndexExpression &expr) override;
    void Visit(const hir::CallExpression &expr) override;
    void Visit(const hir::AccessExpression &expr) override;
    void Visit(const hir::CastExpression &expr) override;
    void Visit(const hir::ESizeofExpression &expr) override;
    void Visit(const hir::TSizeofExpression &expr) override;
    void Visit(const hir::EnumSelectExpression &expr) override;
    void Visit(const hir::VariableExpression &expr) override;
    void Visit(const hir::IntegerExpression &expr) override;
    void Visit(const hir::StringExpression &expr) override;
    void Visit(const hir::CharExpression &expr) override;
    void Visit(const hir::BoolExpression &expr) override;
    void Visit(const hir::StructExpression &expr) override;
    void Visit(const hir::ArrayExpression &expr) override;

private:
    bool success_;
    std::shared_ptr<hir::Type> inferred_;
    CodeGenContext &ctx_;
};

// Evaluate expression as lval, and store result to rax.
class ExprLValGen : public hir::ExpressionVisitor {
public:
    ExprLValGen(CodeGenContext &ctx) : success_(false), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    const std::shared_ptr<hir::Type> &inferred() const { return inferred_; }
    void Visit(const hir::UnaryExpression &expr) override;
    void Visit(const hir::InfixExpression &expr) override;
    void Visit(const hir::IndexExpression &expr) override;
    void Visit(const hir::CallExpression &expr) override;
    void Visit(const hir::AccessExpression &expr) override;
    void Visit(const hir::CastExpression &expr) override;
    void Visit(const hir::ESizeofExpression &expr) override;
    void Visit(const hir::TSizeofExpression &expr) override;
    void Visit(const hir::EnumSelectExpression &expr) override;
    void Visit(const hir::VariableExpression &expr) override;
    void Visit(const hir::IntegerExpression &expr) override;
    void Visit(const hir::StringExpression &expr) override;
    void Visit(const hir::CharExpression &expr) override;
    void Visit(const hir::BoolExpression &expr) override;
    void Visit(const hir::StructExpression &expr) override;
    void Visit(const hir::ArrayExpression &expr) override;

private:
    bool success_;
    std::shared_ptr<hir::Type> inferred_;
    CodeGenContext &ctx_;
};

}  // namespace mini

#endif  // MINI_CODEGEN_EXPR_H_
