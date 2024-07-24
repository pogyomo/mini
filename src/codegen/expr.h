#ifndef MINI_CODEGEN_EXPR_H_
#define MINI_CODEGEN_EXPR_H_

#include "../hir/expr.h"
#include "context.h"

namespace mini {

// Evaluate expression, and store result to rax.
class ExprCodeGen : public hir::ExpressionVisitor {
public:
    ExprCodeGen(CodeGenContext &ctx) : success_(false), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
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
    CodeGenContext &ctx_;
};

}  // namespace mini

#endif  // MINI_CODEGEN_EXPR_H_
