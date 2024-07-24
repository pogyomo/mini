#include "expr.h"

namespace mini {

void ExprCodeGen::Visit(const hir::UnaryExpression &expr) {}
void ExprCodeGen::Visit(const hir::InfixExpression &expr) {}
void ExprCodeGen::Visit(const hir::IndexExpression &expr) {}
void ExprCodeGen::Visit(const hir::CallExpression &expr) {}
void ExprCodeGen::Visit(const hir::AccessExpression &expr) {}
void ExprCodeGen::Visit(const hir::CastExpression &expr) {}
void ExprCodeGen::Visit(const hir::ESizeofExpression &expr) {}
void ExprCodeGen::Visit(const hir::TSizeofExpression &expr) {}
void ExprCodeGen::Visit(const hir::EnumSelectExpression &expr) {}
void ExprCodeGen::Visit(const hir::VariableExpression &expr) {}
void ExprCodeGen::Visit(const hir::IntegerExpression &expr) {}
void ExprCodeGen::Visit(const hir::StringExpression &expr) {}
void ExprCodeGen::Visit(const hir::CharExpression &expr) {}
void ExprCodeGen::Visit(const hir::BoolExpression &expr) {}
void ExprCodeGen::Visit(const hir::StructExpression &expr) {}
void ExprCodeGen::Visit(const hir::ArrayExpression &expr) {}

}  // namespace mini
