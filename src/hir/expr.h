#ifndef MINI_HIR_EXPR_H_
#define MINI_HIR_EXPR_H_

#include <memory>

#include "../span.h"

namespace mini {

namespace hir {

// TODO:
// Implement all expression

class UnaryExpression;
class InfixExpression;

class ExpressionVisitor {
public:
    virtual ~ExpressionVisitor() {}
    virtual void visit(const UnaryExpression& expr) = 0;
    virtual void visit(const InfixExpression& expr) = 0;
};

class Expression {
public:
    Expression(Span span) : span_(span) {}
    virtual void accept(ExpressionVisitor& visitor) const = 0;
    inline Span span() const { return span_; }

private:
    Span span_;
};

class UnaryExpression : public Expression {
public:
    class Op {
    public:
        enum Kind {
            Ref,    // "&"
            Deref,  // "*"
            Minus,  // "-"
            Inv,    // "~"
            Neg,    // "!"
        };
        Op(Kind kind, Span span) : kind_(kind), span_(span) {}
        inline Span span() const { return span_; }
        inline Kind kind() const { return kind_; }

    private:
        const Kind kind_;
        const Span span_;
    };
    UnaryExpression(Op op, std::unique_ptr<Expression>&& expr, Span span)
        : Expression(span), op_(op), expr_(std::move(expr)) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.visit(*this);
    }
    inline Op op() const { return op_; }
    inline const std::unique_ptr<Expression>& expr() const { return expr_; }

private:
    Op op_;
    std::unique_ptr<Expression> expr_;
};

class InfixExpression : public Expression {
public:
    class Op {
    public:
        enum Kind {
            Add,     // "+"
            Sub,     // "-"
            Mul,     // "*"
            Div,     // "/"
            Mod,     // "%"
            Or,      // "||"
            And,     // "&&"
            BitOr,   // "|"
            BitAnd,  // "&"
            BitXor,  // "^"
            Assign,  // "="
            EQ,      // "=="
            NE,      // "!="
            LT,      // "<"
            LE,      // "<="
            GT,      // ">"
            GE,      // ">="
            LShift,  // "<<"
            RShift   // ">>"
        };
        Op(Kind kind, Span span) : kind_(kind), span_(span) {}
        inline Span span() const { return span_; }
        inline Kind kind() const { return kind_; }

    private:
        Kind kind_;
        Span span_;
    };
    InfixExpression(std::unique_ptr<Expression>&& lhs, Op op,
                    std::unique_ptr<Expression>&& rhs, Span span)
        : Expression(span),
          lhs_(std::move(lhs)),
          op_(op),
          rhs_(std::move(rhs)) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.visit(*this);
    }
    inline const std::unique_ptr<Expression>& lhs() const { return lhs_; }
    inline Op op() const { return op_; }
    inline const std::unique_ptr<Expression>& rhs() const { return rhs_; }

private:
    std::unique_ptr<Expression> lhs_;
    Op op_;
    std::unique_ptr<Expression> rhs_;
};

}  // namespace hir

}  // namespace mini

#endif  // MINI_HIR_EXPR_H_
