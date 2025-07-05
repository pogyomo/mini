#include "unused.h"

#include <set>

#include "../context.h"
#include "../report.h"

namespace mini {

namespace hiropt {

bool IsVariableExpression(const std::unique_ptr<hir::Expression>& expr) {
    struct Helper : public hir::ExpressionVisitor {
        bool success_ = false;
        void Visit(const hir::UnaryExpression&) {}
        void Visit(const hir::InfixExpression&) {}
        void Visit(const hir::IndexExpression&) {}
        void Visit(const hir::CallExpression&) {}
        void Visit(const hir::AccessExpression&) {}
        void Visit(const hir::CastExpression&) {}
        void Visit(const hir::ESizeofExpression&) {}
        void Visit(const hir::TSizeofExpression&) {}
        void Visit(const hir::EnumSelectExpression&) {}
        void Visit(const hir::VariableExpression&) { success_ = true; }
        void Visit(const hir::IntegerExpression&) {}
        void Visit(const hir::StringExpression&) {}
        void Visit(const hir::CharExpression&) {}
        void Visit(const hir::BoolExpression&) {}
        void Visit(const hir::NullPtrExpression&) {}
        void Visit(const hir::StructExpression&) {}
        void Visit(const hir::ArrayExpression&) {}
    };
    Helper helper;
    expr->Accept(helper);
    return helper.success_;
}

class UsedVariableCollectorExpr : public hir::ExpressionVisitor {
public:
    UsedVariableCollectorExpr(bool full = false) : full_(full), used_vars_() {}
    const std::set<std::string>& used_vars() { return used_vars_; }
    void Visit(const hir::UnaryExpression& expr) {
        UsedVariableCollectorExpr c;
        expr.expr()->Accept(c);
        used_vars_.insert(c.used_vars_.begin(), c.used_vars_.end());
    }
    void Visit(const hir::InfixExpression& expr) {
        if (expr.op().kind() == hir::InfixExpression::Op::Assign &&
            IsVariableExpression(expr.lhs()) && !full_) {
            UsedVariableCollectorExpr c;
            expr.rhs()->Accept(c);
            used_vars_.insert(c.used_vars_.begin(), c.used_vars_.end());
        } else {
            UsedVariableCollectorExpr c1, c2;
            expr.lhs()->Accept(c1);
            expr.rhs()->Accept(c2);
            used_vars_.insert(c1.used_vars_.begin(), c1.used_vars_.end());
            used_vars_.insert(c2.used_vars_.begin(), c2.used_vars_.end());
        }
    }
    void Visit(const hir::IndexExpression& expr) {
        UsedVariableCollectorExpr c1, c2;
        expr.expr()->Accept(c1);
        expr.index()->Accept(c2);
        used_vars_.insert(c1.used_vars_.begin(), c1.used_vars_.end());
        used_vars_.insert(c2.used_vars_.begin(), c2.used_vars_.end());
    }
    void Visit(const hir::CallExpression& expr) {
        UsedVariableCollectorExpr c;
        expr.func()->Accept(c);
        used_vars_.insert(c.used_vars_.begin(), c.used_vars_.end());
        for (auto& arg : expr.args()) {
            UsedVariableCollectorExpr c;
            arg->Accept(c);
            used_vars_.insert(c.used_vars_.begin(), c.used_vars_.end());
        }
    }
    void Visit(const hir::AccessExpression& expr) {
        UsedVariableCollectorExpr c;
        expr.expr()->Accept(c);
        used_vars_.insert(c.used_vars_.begin(), c.used_vars_.end());
    }
    void Visit(const hir::CastExpression& expr) {
        UsedVariableCollectorExpr c;
        expr.expr()->Accept(c);
        used_vars_.insert(c.used_vars_.begin(), c.used_vars_.end());
    }
    void Visit(const hir::ESizeofExpression& expr) {
        UsedVariableCollectorExpr c;
        expr.expr()->Accept(c);
        used_vars_.insert(c.used_vars_.begin(), c.used_vars_.end());
    }
    void Visit(const hir::TSizeofExpression&) {}
    void Visit(const hir::EnumSelectExpression&) {}
    void Visit(const hir::VariableExpression& expr) {
        used_vars_.insert(expr.value());
    }
    void Visit(const hir::IntegerExpression&) {}
    void Visit(const hir::StringExpression&) {}
    void Visit(const hir::CharExpression&) {}
    void Visit(const hir::BoolExpression&) {}
    void Visit(const hir::NullPtrExpression&) {}
    void Visit(const hir::StructExpression& expr) {
        for (auto& init : expr.inits()) {
            UsedVariableCollectorExpr c;
            init.value()->Accept(c);
            used_vars_.insert(c.used_vars_.begin(), c.used_vars_.end());
        }
    }
    void Visit(const hir::ArrayExpression& expr) {
        for (auto& init : expr.inits()) {
            UsedVariableCollectorExpr c;
            init->Accept(c);
            used_vars_.insert(c.used_vars_.begin(), c.used_vars_.end());
        }
    }

private:
    bool full_;  // if false, lhs in assign expression is considered as unused.
    std::set<std::string> used_vars_;
};

class UsedVariableCollectorStmt : public hir::StatementVisitor {
public:
    UsedVariableCollectorStmt() : used_vars_() {}
    const std::set<std::string>& used_vars() { return used_vars_; }
    void Visit(const hir::ExpressionStatement& stmt) {
        UsedVariableCollectorExpr c;
        stmt.expr()->Accept(c);
        used_vars_.insert(c.used_vars().begin(), c.used_vars().end());
    }
    void Visit(const hir::ReturnStatement& stmt) {
        if (stmt.ret_value()) {
            UsedVariableCollectorExpr c;
            stmt.ret_value().value()->Accept(c);
            used_vars_.insert(c.used_vars().begin(), c.used_vars().end());
        }
    }
    void Visit(const hir::BreakStatement&) {}
    void Visit(const hir::ContinueStatement&) {}
    void Visit(const hir::WhileStatement& stmt) {
        UsedVariableCollectorExpr c1;
        stmt.cond()->Accept(c1);
        used_vars_.insert(c1.used_vars().begin(), c1.used_vars().end());

        UsedVariableCollectorStmt c2;
        stmt.body()->Accept(c2);
        used_vars_.insert(c2.used_vars_.begin(), c2.used_vars_.end());
    }
    void Visit(const hir::IfStatement& stmt) {
        UsedVariableCollectorExpr c1;
        stmt.cond()->Accept(c1);
        used_vars_.insert(c1.used_vars().begin(), c1.used_vars().end());

        UsedVariableCollectorStmt c2;
        stmt.then_body()->Accept(c2);
        used_vars_.insert(c2.used_vars_.begin(), c2.used_vars_.end());

        if (stmt.else_body()) {
            UsedVariableCollectorStmt c3;
            stmt.else_body().value()->Accept(c3);
            used_vars_.insert(c3.used_vars_.begin(), c3.used_vars_.end());
        }
    }
    void Visit(const hir::BlockStatement& stmt) {
        for (const auto& stmt : stmt.stmts()) {
            UsedVariableCollectorStmt c;
            stmt->Accept(c);
            used_vars_.insert(c.used_vars_.begin(), c.used_vars_.end());
        }
    }

private:
    std::set<std::string> used_vars_;
};

// Remove statement that contains unused variable
class StatementRemover : public hir::StatementVisitorMut {
public:
    StatementRemover(const std::set<std::string>& used_vars)
        : should_remove_(false), used_vars_(used_vars) {}
    void Visit(hir::ExpressionStatement& stmt) {
        UsedVariableCollectorExpr c(true);
        stmt.expr()->Accept(c);

        for (auto& name : c.used_vars()) {
            if (used_vars_.find(name) == used_vars_.end()) {
                should_remove_ = true;
            }
        }
    }
    void Visit(hir::ReturnStatement& stmt) {
        if (!stmt.ret_value()) return;
        UsedVariableCollectorExpr c;
        stmt.ret_value().value()->Accept(c);

        for (auto& name : c.used_vars()) {
            if (used_vars_.find(name) == used_vars_.end()) {
                should_remove_ = true;
            }
        }
    }
    void Visit(hir::BreakStatement&) {}
    void Visit(hir::ContinueStatement&) {}
    void Visit(hir::WhileStatement& stmt) {
        StatementRemover remove(used_vars_);
        stmt.body()->Accept(remove);
    }
    void Visit(hir::IfStatement& stmt) {
        StatementRemover remove(used_vars_);
        stmt.then_body()->Accept(remove);
        if (stmt.else_body()) {
            StatementRemover remove(used_vars_);
            stmt.else_body().value()->Accept(remove);
        }
    }
    void Visit(hir::BlockStatement& stmt) {
        for (auto it = stmt.stmts().begin(); it != stmt.stmts().end();) {
            StatementRemover remove(used_vars_);
            it->get()->Accept(remove);
            if (remove.should_remove_) {
                it = stmt.stmts().erase(it);
            } else {
                ++it;
            }
        }
        should_remove_ = stmt.stmts().size() == 0;
    }

private:
    bool should_remove_;
    std::set<std::string> used_vars_;
};

class UnusedVariableRemover : public hir::DeclarationVisitorMut {
public:
    UnusedVariableRemover(Context& ctx) : removed_(false), ctx_(ctx) {}
    explicit operator bool() const { return removed_; }
    void Visit(hir::StructDeclaration&) {}
    void Visit(hir::EnumDeclaration&) {}
    void Visit(hir::FunctionDeclaration& decl) {
        if (!decl.body()) {
            return;
        }

        UsedVariableCollectorStmt c;
        decl.body()->Accept(c);

        // Remove unused variable declaration
        for (auto it = decl.decls().begin(); it != decl.decls().end();) {
            bool exists = false;
            for (auto& name : c.used_vars()) {
                if (it->name().value() == name) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                ReportInfo info(it->span(), "unused variable", "");
                Report(ctx_, ReportLevel::Warn, info);
                it = decl.decls().erase(it);
                removed_ = true;
            } else {
                ++it;
            }
        }

        // Remove statements that holds unused variable
        StatementRemover remove(c.used_vars());
        decl.body()->Accept(remove);

        // Report unused parameter
        for (auto& param : decl.params()) {
            bool exists = false;
            for (auto& name : c.used_vars()) {
                if (param.name().value() == name) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                ReportInfo info(param.span(), "unused parameter", "");
                Report(ctx_, ReportLevel::Warn, info);
            }
        }
    }

private:
    bool removed_;
    Context& ctx_;
};

void RemoveUnusedVariable(Context& ctx, hir::Root& root) {
    while (true) {
        bool continue_ = false;
        for (auto& decl : root.decls()) {
            UnusedVariableRemover remove(ctx);
            decl->Accept(remove);
            if (remove) continue_ = true;
        }
        if (!continue_) break;
    }
}

}  // namespace hiropt

}  // namespace mini
