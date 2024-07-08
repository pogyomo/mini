#ifndef MINI_HIR_STMT_H_
#define MINI_HIR_STMT_H_

#include <memory>
#include <optional>
#include <vector>

#include "../span.h"
#include "expr.h"
#include "type.h"

namespace mini {

namespace hir {

class ExpressionStatement;
class ReturnStatement;
class BreakStatement;
class ContinueStatement;
class WhileStatement;
class IfStatement;
class BlockStatement;

class StatementVisitor {
public:
    virtual ~StatementVisitor() {}
    virtual void visit(const ExpressionStatement &stmt) = 0;
    virtual void visit(const ReturnStatement &stmt) = 0;
    virtual void visit(const BreakStatement &stmt) = 0;
    virtual void visit(const ContinueStatement &stmt) = 0;
    virtual void visit(const WhileStatement &stmt) = 0;
    virtual void visit(const IfStatement &stmt) = 0;
    virtual void visit(const BlockStatement &stmt) = 0;
};

class Statement {
public:
    Statement(Span span) : span_(span) {}
    virtual void accept(StatementVisitor &visitor) const = 0;
    inline Span span() const { return span_; }

private:
    Span span_;
};

class ExpressionStatement : public Statement {
public:
    ExpressionStatement(std::unique_ptr<Expression> &&expr, Span span)
        : Statement(span), expr_(std::move(expr)) {}
    inline void accept(StatementVisitor &visitor) const override {
        visitor.visit(*this);
    }
    inline const std::unique_ptr<Expression> &expr() const { return expr_; }

private:
    std::unique_ptr<Expression> expr_;
};

class ReturnStatement : public Statement {
public:
    ReturnStatement(std::unique_ptr<Expression> &&ret_value, Span span)
        : Statement(span), ret_value_(std::move(ret_value)) {}
    inline void accept(StatementVisitor &visitor) const override {
        visitor.visit(*this);
    }
    inline const std::unique_ptr<Expression> &ret_value() const {
        return ret_value_;
    }

private:
    std::unique_ptr<Expression> ret_value_;
};

class BreakStatement : public Statement {
public:
    BreakStatement(Span span) : Statement(span) {}
    inline void accept(StatementVisitor &visitor) const override {
        visitor.visit(*this);
    }
};

class ContinueStatement : public Statement {
public:
    ContinueStatement(Span span) : Statement(span) {}
    inline void accept(StatementVisitor &visitor) const override {
        visitor.visit(*this);
    }
};

class WhileStatement : public Statement {
public:
    WhileStatement(std::unique_ptr<Expression> &&cond,
                   std::unique_ptr<Statement> &&body, Span span)
        : Statement(span), cond_(std::move(cond)), body_(std::move(body)) {}
    inline void accept(StatementVisitor &visitor) const override {
        visitor.visit(*this);
    }
    inline const std::unique_ptr<Expression> &cond() const { return cond_; }
    inline const std::unique_ptr<Statement> &body() const { return body_; }

private:
    std::unique_ptr<Expression> cond_;
    std::unique_ptr<Statement> body_;
};

class IfStatement : public Statement {
public:
    IfStatement(std::unique_ptr<Expression> &&cond,
                std::unique_ptr<Statement> &&then_body,
                std::optional<std::unique_ptr<Statement>> &&else_body,
                Span span)
        : Statement(span),
          cond_(std::move(cond)),
          then_body_(std::move(then_body)),
          else_body_(std::move(else_body)) {}
    inline void accept(StatementVisitor &visitor) const override {
        visitor.visit(*this);
    }
    inline const std::unique_ptr<Expression> &cond() const { return cond_; }
    inline const std::unique_ptr<Statement> &then_body() const {
        return then_body_;
    }
    inline const std::optional<std::unique_ptr<Statement>> &else_body() const {
        return else_body_;
    }

private:
    std::unique_ptr<Expression> cond_;
    std::unique_ptr<Statement> then_body_;
    std::optional<std::unique_ptr<Statement>> else_body_;
};

class VariableDeclarationName {
public:
    VariableDeclarationName(std::string &&value, Span span)
        : value_(std::move(value)), span_(span) {}
    inline const std::string &value() const { return value_; }
    inline Span span() const { return span_; }

private:
    std::string value_;
    Span span_;
};

class VariableDeclaration {
public:
    VariableDeclaration(const std::shared_ptr<Type> &type,
                        VariableDeclarationName &&name, Span span)
        : type_(std::move(type)), name_(std::move(name)), span_(span) {}
    inline const std::shared_ptr<Type> &type() const { return type_; }
    inline const VariableDeclarationName &name() const { return name_; }
    inline Span span() const { return span_; }

private:
    std::shared_ptr<Type> type_;
    VariableDeclarationName name_;
    Span span_;
};

// TODO:
// Is it necessary to separete mixed declaration and statements from
// ast::BlockStatement?
class BlockStatement : public Statement {
public:
    BlockStatement(std::vector<VariableDeclaration> &&decls,
                   std::vector<std::unique_ptr<Statement>> &&stmts, Span span)
        : Statement(span), decls_(std::move(decls)), stmts_(std::move(stmts)) {}
    inline void accept(StatementVisitor &visitor) const override {
        visitor.visit(*this);
    }
    inline const std::vector<VariableDeclaration> &decls() const {
        return decls_;
    }
    inline const std::vector<std::unique_ptr<Statement>> &stmts() const {
        return stmts_;
    }

private:
    std::vector<VariableDeclaration> decls_;

    // Variables in statements must be checked each is used after it was
    // declared, as this representation lost the infomations.
    std::vector<std::unique_ptr<Statement>> stmts_;
};

}  // namespace hir

}  // namespace mini

#endif  // MINI_HIR_STMT_H_
