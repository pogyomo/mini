#ifndef MINI_EVAL_H_
#define MINI_EVAL_H_

#include "ast/expr.h"
#include "context.h"

namespace mini {

class ConstEval : public ast::ExpressionVisitor {
public:
    ConstEval(Context &ctx) : success_(false), value_(0), ctx_(ctx) {}
    explicit operator bool() { return success_; }
    uint64_t value() const { return value_; }
    void Visit(const ast::UnaryExpression &expr) override;
    void Visit(const ast::InfixExpression &expr) override;
    void Visit(const ast::IndexExpression &expr) override;
    void Visit(const ast::CallExpression &expr) override;
    void Visit(const ast::AccessExpression &expr) override;
    void Visit(const ast::CastExpression &expr) override;
    void Visit(const ast::ESizeofExpression &expr) override;
    void Visit(const ast::TSizeofExpression &expr) override;
    void Visit(const ast::EnumSelectExpression &expr) override;
    void Visit(const ast::VariableExpression &expr) override;
    void Visit(const ast::IntegerExpression &expr) override;
    void Visit(const ast::StringExpression &expr) override;
    void Visit(const ast::CharExpression &expr) override;
    void Visit(const ast::BoolExpression &expr) override;
    void Visit(const ast::StructExpression &expr) override;
    void Visit(const ast::ArrayExpression &expr) override;

private:
    void report_error(Span span, const std::string &what);

    bool success_;
    uint64_t value_;
    Context &ctx_;
};

};  // namespace mini

#endif  // MINI_EVAL_H_
