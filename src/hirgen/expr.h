#ifndef MINI_HIRGEN_EXPR_H_
#define MINI_HIRGEN_EXPR_H_

#include "../ast/expr.h"
#include "../hir/expr.h"
#include "context.h"

namespace mini {

class ExprHirGen : public ast::ExpressionVisitor {
public:
    ExprHirGen(HirGenContext &ctx) : success_(false), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    std::unique_ptr<hir::Expression> &expr() { return expr_; }
    void visit(const ast::UnaryExpression &expr) override;
    void visit(const ast::InfixExpression &expr) override;
    void visit(const ast::IndexExpression &expr) override;
    void visit(const ast::CallExpression &expr) override;
    void visit(const ast::AccessExpression &expr) override;
    void visit(const ast::CastExpression &expr) override;
    void visit(const ast::ESizeofExpression &expr) override;
    void visit(const ast::TSizeofExpression &expr) override;
    void visit(const ast::EnumSelectExpression &expr) override;
    void visit(const ast::VariableExpression &expr) override;
    void visit(const ast::IntegerExpression &expr) override;
    void visit(const ast::StringExpression &expr) override;
    void visit(const ast::CharExpression &expr) override;
    void visit(const ast::BoolExpression &expr) override;
    void visit(const ast::StructExpression &expr) override;
    void visit(const ast::ArrayExpression &expr) override;

private:
    bool success_;
    std::unique_ptr<hir::Expression> expr_;
    HirGenContext &ctx_;
};

}  // namespace mini

#endif  // MINI_HIRGEN_EXPR_H_
