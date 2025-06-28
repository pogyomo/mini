#ifndef MINI_AST_STMT_H_
#define MINI_AST_STMT_H_

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "node.h"
#include "type.h"

namespace mini {

namespace ast {

class Expression;

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
    virtual void Visit(const ExpressionStatement& stmt) = 0;
    virtual void Visit(const ReturnStatement& stmt) = 0;
    virtual void Visit(const BreakStatement& stmt) = 0;
    virtual void Visit(const ContinueStatement& stmt) = 0;
    virtual void Visit(const WhileStatement& stmt) = 0;
    virtual void Visit(const IfStatement& stmt) = 0;
    virtual void Visit(const BlockStatement& stmt) = 0;
};

class Statement : public Node {
public:
    virtual ~Statement() {}
    virtual void Accept(StatementVisitor& visitor) const = 0;
};

class ExpressionStatement : public Statement {
public:
    ExpressionStatement(std::unique_ptr<Expression>&& expr,
                        Semicolon semicolon);
    ~ExpressionStatement();
    inline void Accept(StatementVisitor& visitor) const override {
        visitor.Visit(*this);
    }
    Span span() const override;
    inline const std::unique_ptr<Expression>& expr() const { return expr_; }
    inline Semicolon semicolon() const { return semicolon_; }

private:
    std::unique_ptr<Expression> expr_;
    Semicolon semicolon_;
};

class ReturnStatement : public Statement {
public:
    ReturnStatement(Return return_kw,
                    std::optional<std::unique_ptr<Expression>>&& expr,
                    Semicolon semicolon);
    ~ReturnStatement();
    inline void Accept(StatementVisitor& visitor) const override {
        visitor.Visit(*this);
    }
    inline Span span() const override {
        return return_kw_.span() + semicolon_.span();
    }
    inline Return return_kw() const { return return_kw_; }
    inline const std::optional<std::unique_ptr<Expression>>& expr() const {
        return expr_;
    }
    inline Semicolon semicolon() const { return semicolon_; }

private:
    Return return_kw_;
    std::optional<std::unique_ptr<Expression>> expr_;
    Semicolon semicolon_;
};

class BreakStatement : public Statement {
public:
    BreakStatement(Break break_kw, Semicolon semicolon)
        : break_kw_(break_kw), semicolon_(semicolon) {}
    inline void Accept(StatementVisitor& visitor) const override {
        visitor.Visit(*this);
    }
    inline Span span() const override {
        return break_kw_.span() + semicolon_.span();
    }
    inline Break break_kw() const { return break_kw_; }
    inline Semicolon semicolon() const { return semicolon_; }

private:
    Break break_kw_;
    Semicolon semicolon_;
};

class ContinueStatement : public Statement {
public:
    ContinueStatement(Continue continue_kw, Semicolon semicolon)
        : continue_kw_(continue_kw), semicolon_(semicolon) {}
    inline void Accept(StatementVisitor& visitor) const override {
        visitor.Visit(*this);
    }
    inline Span span() const override {
        return continue_kw_.span() + semicolon_.span();
    }
    inline Continue continue_kw() const { return continue_kw_; }
    inline Semicolon semicolon() const { return semicolon_; }

private:
    Continue continue_kw_;
    Semicolon semicolon_;
};

class WhileStatement : public Statement {
public:
    WhileStatement(While while_kw, LParen lparen,
                   std::unique_ptr<Expression>&& cond, RParen rparen,
                   std::unique_ptr<Statement>&& body);
    ~WhileStatement();
    inline void Accept(StatementVisitor& visitor) const override {
        visitor.Visit(*this);
    }
    inline Span span() const override {
        return while_kw_.span() + body_->span();
    }
    inline While while_kw() const { return while_kw_; }
    inline LParen lparen() const { return lparen_; }
    inline const std::unique_ptr<Expression>& cond() const { return cond_; }
    inline RParen rparen() const { return rparen_; }
    inline const std::unique_ptr<Statement>& body() const { return body_; }

private:
    While while_kw_;
    LParen lparen_;
    std::unique_ptr<Expression> cond_;
    RParen rparen_;
    std::unique_ptr<Statement> body_;
};

class IfStatementElseClause : public Node {
public:
    IfStatementElseClause(Else else_kw, std::unique_ptr<Statement>&& body)
        : else_kw_(else_kw), body_(std::move(body)) {}
    inline Span span() const override {
        return else_kw_.span() + body_->span();
    }
    inline Else else_kw() const { return else_kw_; }
    inline const std::unique_ptr<Statement>& body() const { return body_; }

private:
    Else else_kw_;
    std::unique_ptr<Statement> body_;
};

class IfStatement : public Statement {
public:
    IfStatement(If if_kw, LParen lparen, std::unique_ptr<Expression>&& cond,
                RParen rparen, std::unique_ptr<Statement>&& body,
                std::optional<IfStatementElseClause>&& else_clause);
    ~IfStatement();
    inline void Accept(StatementVisitor& visitor) const override {
        visitor.Visit(*this);
    }
    inline Span span() const override {
        return else_clause_ ? if_kw_.span() + else_clause_->span()
                            : if_kw_.span() + body_->span();
    }
    inline If if_kw() const { return if_kw_; }
    inline LParen lparen() const { return lparen_; }
    inline const std::unique_ptr<Expression>& cond() const { return cond_; }
    inline RParen rparen() const { return rparen_; }
    inline const std::unique_ptr<Statement>& body() const { return body_; }
    inline const std::optional<IfStatementElseClause>& else_clause() const {
        return else_clause_;
    }

private:
    If if_kw_;
    LParen lparen_;
    std::unique_ptr<Expression> cond_;
    RParen rparen_;
    std::unique_ptr<Statement> body_;
    std::optional<IfStatementElseClause> else_clause_;
};

class VariableName : public Node {
public:
    VariableName(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

class VariableInit : public Node {
public:
    VariableInit(VariableInit&& other);
    VariableInit(Assign assign, std::unique_ptr<Expression>&& expr);
    ~VariableInit();
    Span span() const override;
    inline Assign assign() const { return assign_; }
    inline const std::unique_ptr<Expression>& expr() const { return expr_; }

private:
    Assign assign_;
    std::unique_ptr<Expression> expr_;
};

class VariableDeclarationBody : public Node {
public:
    VariableDeclarationBody(VariableName&& name, Colon colon,
                            const std::shared_ptr<Type>& type,
                            std::optional<VariableInit>&& init);
    inline Span span() const override {
        return init_.has_value() ? name_.span() + init_->span() : name_.span();
    }
    inline const VariableName& name() const { return name_; }
    inline Colon colon() const { return colon_; }
    inline const std::shared_ptr<Type>& type() const { return type_; }
    inline const std::optional<VariableInit>& init() const { return init_; }

private:
    VariableName name_;
    Colon colon_;
    std::shared_ptr<Type> type_;
    std::optional<VariableInit> init_;
};

class VariableDeclarations : public Node {
public:
    VariableDeclarations(Let let_kw,
                         std::vector<VariableDeclarationBody>&& names,
                         Semicolon semicolon)
        : let_kw_(let_kw), bodies_(std::move(names)), semicolon_(semicolon) {}
    inline Span span() const override {
        return let_kw_.span() + semicolon_.span();
    }
    inline const std::vector<VariableDeclarationBody>& bodies() const {
        return bodies_;
    }
    inline Semicolon semicolon() const { return semicolon_; }

private:
    Let let_kw_;
    std::vector<VariableDeclarationBody> bodies_;
    Semicolon semicolon_;
};

class BlockStatementItem : public Node {
public:
    BlockStatementItem(VariableDeclarations&& decl) : item_(std::move(decl)) {}
    BlockStatementItem(std::unique_ptr<Statement>&& stmt)
        : item_(std::move(stmt)) {}
    Span span() const override {
        if (item_.index() == 0) {
            return std::get<0>(item_).span();
        } else {
            return std::get<1>(item_)->span();
        }
    }
    bool IsDecl() const { return item_.index() == 0; }
    bool IsStmt() const { return item_.index() == 1; }
    const VariableDeclarations& decl() const { return std::get<0>(item_); }
    const std::unique_ptr<Statement>& stmt() const {
        return std::get<1>(item_);
    }

private:
    std::variant<VariableDeclarations, std::unique_ptr<Statement>> item_;
};

class BlockStatement : public Statement {
public:
    BlockStatement(LCurly lcurly, std::vector<BlockStatementItem>&& items,
                   RCurly rcurly)
        : lcurly_(lcurly), items_(std::move(items)), rcurly_(rcurly) {}
    inline void Accept(StatementVisitor& visitor) const override {
        visitor.Visit(*this);
    }
    inline Span span() const override {
        return lcurly_.span() + rcurly_.span();
    }
    inline LCurly lcurly() const { return lcurly_; }
    inline const std::vector<BlockStatementItem>& items() const {
        return items_;
    }
    inline RCurly rcurly() const { return rcurly_; }

private:
    LCurly lcurly_;
    std::vector<BlockStatementItem> items_;
    RCurly rcurly_;
};

};  // namespace ast

};  // namespace mini

#endif  // MINI_AST_STMT_H_
