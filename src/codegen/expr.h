#ifndef MINI_CODEGEN_EXPR_H_
#define MINI_CODEGEN_EXPR_H_

#include <memory>
#include <optional>

#include "../hir/expr.h"
#include "asm.h"
#include "context.h"

namespace mini {

// Evaluate expression as rvalue, and store result to stack
// Allocated stack is always aligned to 8 bytes.
class ExprRValGen : public hir::ExpressionVisitor {
public:
    ExprRValGen(CodeGenContext &ctx)
        : success_(false), array_base_type_(std::nullopt), ctx_(ctx) {}
    ExprRValGen(
        CodeGenContext &ctx,
        const std::optional<std::shared_ptr<hir::Type>> &array_base_type)
        : success_(false), array_base_type_(array_base_type), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    const std::shared_ptr<hir::Type> &inferred() const { return inferred_; }
    const std::optional<std::shared_ptr<hir::Type>> &arary_base_type() const {
        return array_base_type_;
    }
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

    // What the base type of array is expected.
    // Only used for generate expression of hir::ArrayExpression
    std::optional<std::shared_ptr<hir::Type>> array_base_type_;

    std::shared_ptr<hir::Type> inferred_;
    CodeGenContext &ctx_;
};

// Evaluate expression as lvalue, and store result to stack.
// This always generates address, so always allocate 8 bytes and so stack
// aligned 8 bytes.
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

// Implicitly convert value of type `from` to type `to` which in top of stack
bool ImplicitlyConvertValueInStack(CodeGenContext &ctx, Span value_span,
                                   const std::shared_ptr<hir::Type> &from,
                                   const std::shared_ptr<hir::Type> &to);

}  // namespace mini

#endif  // MINI_CODEGEN_EXPR_H_
