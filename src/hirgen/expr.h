#ifndef MINI_HIRGEN_EXPR_H_
#define MINI_HIRGEN_EXPR_H_

#include <cstdint>
#include <optional>

#include "../ast/expr.h"
#include "../hir/expr.h"
#include "context.h"

namespace mini {

class ExprHirGen : public ast::ExpressionVisitor {
public:
    ExprHirGen(HirGenContext &ctx, bool is_root = true)
        : success_(false), is_root_(is_root), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    bool IsRoot() const { return is_root_; }
    std::unique_ptr<hir::Expression> &expr() { return expr_; }
    std::optional<uint64_t> array_size() const { return array_size_; }
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
    void Visit(const ast::NullPtrExpression &expr) override;
    void Visit(const ast::StructExpression &expr) override;
    void Visit(const ast::ArrayExpression &expr) override;

private:
    bool success_;
    bool is_root_;
    std::unique_ptr<hir::Expression> expr_;
    std::optional<uint64_t> array_size_;
    HirGenContext &ctx_;
};

}  // namespace mini

#endif  // MINI_HIRGEN_EXPR_H_
