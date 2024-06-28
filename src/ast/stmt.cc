#include "stmt.h"

#include "expr.h"

ExpressionStatement::ExpressionStatement(std::unique_ptr<Expression>&& expr,
                                         Semicolon semicolon)
    : expr_(std::move(expr)), semicolon_(semicolon) {}

ExpressionStatement::~ExpressionStatement() = default;

inline Span ExpressionStatement::span() const {
    return expr_->span() + semicolon_.span();
}

inline Span VariableInit::span() const {
    return assign_.span() + expr_->span();
}

ReturnStatement::ReturnStatement(
    Return return_kw, std::optional<std::unique_ptr<Expression>>&& expr,
    Semicolon semicolon)
    : return_kw_(return_kw), expr_(std::move(expr)), semicolon_(semicolon) {}

ReturnStatement::~ReturnStatement() = default;

WhileStatement::WhileStatement(While while_kw, LParen lparen,
                               std::unique_ptr<Expression>&& cond,
                               RParen rparen, std::unique_ptr<Statement>&& body)
    : while_kw_(while_kw),
      lparen_(lparen),
      cond_(std::move(cond)),
      rparen_(rparen),
      body_(std::move(body)) {}

WhileStatement::~WhileStatement() = default;

IfStatement::IfStatement(If if_kw, LParen lparen,
                         std::unique_ptr<Expression>&& cond, RParen rparen,
                         std::unique_ptr<Statement>&& body,
                         std::optional<IfStatementElseClause>&& else_clause)
    : if_kw_(if_kw),
      lparen_(lparen),
      cond_(std::move(cond)),
      rparen_(rparen),
      body_(std::move(body)),
      else_clause_(std::move(else_clause)) {}

IfStatement::~IfStatement() = default;

VariableInit::VariableInit(VariableInit&& other) = default;

VariableInit::VariableInit(Assign assign, std::unique_ptr<Expression>&& expr)
    : assign_(assign), expr_(std::move(expr)) {}

VariableInit::~VariableInit() = default;

VariableDeclarationBody::VariableDeclarationBody(
    VariableName&& name, Colon colon, const std::shared_ptr<Type>& type,
    std::optional<VariableInit>&& init)
    : name_(std::move(name)),
      colon_(colon),
      type_(std::move(type)),
      init_(std::move(init)) {}
