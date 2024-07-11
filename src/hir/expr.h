#ifndef MINI_HIR_EXPR_H_
#define MINI_HIR_EXPR_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../span.h"
#include "printable.h"
#include "type.h"

namespace mini {

namespace hir {

class UnaryExpression;
class InfixExpression;
class IndexExpression;
class CallExpression;
class AccessExpression;
class CastExpression;
class ESizeofExpression;
class TSizeofExpression;
class EnumSelectExpression;
class VariableExpression;
class IntegerExpression;
class StringExpression;
class CharExpression;
class BoolExpression;
class StructExpression;
class ArrayExpression;

class ExpressionVisitor {
public:
    virtual ~ExpressionVisitor() {}
    virtual void Visit(const UnaryExpression& expr) = 0;
    virtual void Visit(const InfixExpression& expr) = 0;
    virtual void Visit(const IndexExpression& expr) = 0;
    virtual void Visit(const CallExpression& expr) = 0;
    virtual void Visit(const AccessExpression& expr) = 0;
    virtual void Visit(const CastExpression& expr) = 0;
    virtual void Visit(const ESizeofExpression& expr) = 0;
    virtual void Visit(const TSizeofExpression& expr) = 0;
    virtual void Visit(const EnumSelectExpression& expr) = 0;
    virtual void Visit(const VariableExpression& expr) = 0;
    virtual void Visit(const IntegerExpression& expr) = 0;
    virtual void Visit(const StringExpression& expr) = 0;
    virtual void Visit(const CharExpression& expr) = 0;
    virtual void Visit(const BoolExpression& expr) = 0;
    virtual void Visit(const StructExpression& expr) = 0;
    virtual void Visit(const ArrayExpression& expr) = 0;
};

class Expression : public Printable {
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
        std::string to_string() const;
        inline Span span() const { return span_; }
        inline Kind kind() const { return kind_; }

    private:
        const Kind kind_;
        const Span span_;
    };
    UnaryExpression(Op op, std::unique_ptr<Expression>&& expr, Span span)
        : Expression(span), op_(op), expr_(std::move(expr)) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    void Print(PrintableContext& ctx) const override;
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
        std::string to_string() const;
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
        return visitor.Visit(*this);
    }
    void Print(PrintableContext& ctx) const override;
    inline const std::unique_ptr<Expression>& lhs() const { return lhs_; }
    inline Op op() const { return op_; }
    inline const std::unique_ptr<Expression>& rhs() const { return rhs_; }

private:
    std::unique_ptr<Expression> lhs_;
    Op op_;
    std::unique_ptr<Expression> rhs_;
};

class IndexExpression : public Expression {
public:
    IndexExpression(std::unique_ptr<Expression>&& expr,
                    std::unique_ptr<Expression>&& index, Span span)
        : Expression(span), expr_(std::move(expr)), index_(std::move(index)) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    void Print(PrintableContext& ctx) const override;
    inline const std::unique_ptr<Expression>& expr() const { return expr_; }
    inline const std::unique_ptr<Expression>& index() const { return index_; }

private:
    std::unique_ptr<Expression> expr_;
    std::unique_ptr<Expression> index_;
};

class CallExpression : public Expression {
public:
    CallExpression(std::unique_ptr<Expression>&& func,
                   std::vector<std::unique_ptr<Expression>>&& args, Span span)
        : Expression(span), func_(std::move(func)), args_(std::move(args)) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    void Print(PrintableContext& ctx) const override;
    inline const std::unique_ptr<Expression>& func() const { return func_; }
    inline const std::vector<std::unique_ptr<Expression>>& args() const {
        return args_;
    }

private:
    std::unique_ptr<Expression> func_;
    std::vector<std::unique_ptr<Expression>> args_;
};

class AccessExpressionField {
public:
    AccessExpressionField(std::string&& value, Span span)
        : value_(std::move(value)), span_(span) {}
    inline const std::string& value() const { return value_; }
    inline Span span() const { return span_; }

private:
    std::string value_;
    Span span_;
};

class AccessExpression : public Expression {
public:
    AccessExpression(std::unique_ptr<Expression>&& func,
                     AccessExpressionField&& field, Span span)
        : Expression(span), expr_(std::move(func)), field_(std::move(field)) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    void Print(PrintableContext& ctx) const override;
    inline const std::unique_ptr<Expression>& func() const { return expr_; }
    inline const AccessExpressionField& field() const { return field_; }

private:
    std::unique_ptr<Expression> expr_;
    AccessExpressionField field_;
};

class CastExpression : public Expression {
public:
    CastExpression(std::unique_ptr<Expression>&& expr,
                   const std::shared_ptr<Type>& cast_type, Span span)
        : Expression(span),
          expr_(std::move(expr)),
          cast_type_(std::move(cast_type)) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    void Print(PrintableContext& ctx) const override;
    inline const std::unique_ptr<Expression>& expr() const { return expr_; }
    inline const std::shared_ptr<Type>& cast_type() const { return cast_type_; }

private:
    std::unique_ptr<Expression> expr_;
    std::shared_ptr<Type> cast_type_;
};

class ESizeofExpression : public Expression {
public:
    ESizeofExpression(std::unique_ptr<Expression>&& expr, Span span)
        : Expression(span), expr_(std::move(expr)) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    void Print(PrintableContext& ctx) const override;
    inline const std::unique_ptr<Expression>& expr() const { return expr_; }

private:
    std::unique_ptr<Expression> expr_;
};

class TSizeofExpression : public Expression {
public:
    TSizeofExpression(const std::shared_ptr<Type>& type, Span span)
        : Expression(span), type_(type) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    void Print(PrintableContext& ctx) const override;
    inline const std::shared_ptr<Type>& type() const { return type_; }

private:
    std::shared_ptr<Type> type_;
};

class EnumSelectExpressionSrc {
public:
    EnumSelectExpressionSrc(std::string&& value, Span span)
        : value_(std::move(value)), span_(span) {}
    inline const std::string& value() const { return value_; }
    inline Span span() const { return span_; }

private:
    std::string value_;
    Span span_;
};

class EnumSelectExpressionDst {
public:
    EnumSelectExpressionDst(std::string&& value, Span span)
        : value_(std::move(value)), span_(span) {}
    inline const std::string& value() const { return value_; }
    inline Span span() const { return span_; }

private:
    std::string value_;
    Span span_;
};

class EnumSelectExpression : public Expression {
public:
    EnumSelectExpression(EnumSelectExpressionSrc&& src,
                         EnumSelectExpressionDst&& dst, Span span)
        : Expression(span), src_(std::move(src)), dst_(std::move(dst)) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline void Print(PrintableContext& ctx) const override {
        ctx.printer().Print("{}::{}", src_.value(), dst_.value());
    }
    inline const EnumSelectExpressionSrc& src() const { return src_; }
    inline const EnumSelectExpressionDst& dst() const { return dst_; }

private:
    EnumSelectExpressionSrc src_;
    EnumSelectExpressionDst dst_;
};

class VariableExpression : public Expression {
public:
    VariableExpression(std::string&& value, Span span)
        : Expression(span), value_(std::move(value)) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline void Print(PrintableContext& ctx) const override {
        ctx.printer().Print(value_);
    }
    inline const std::string& value() const { return value_; }

private:
    std::string value_;
};

class IntegerExpression : public Expression {
public:
    IntegerExpression(uint64_t value, Span span)
        : Expression(span), value_(value) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline void Print(PrintableContext& ctx) const override {
        ctx.printer().Print("{}", value_);
    }
    inline uint64_t value() const { return value_; }

private:
    uint64_t value_;
};

class StringExpression : public Expression {
public:
    StringExpression(std::string&& value, Span span)
        : Expression(span), value_(std::move(value)) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline void Print(PrintableContext& ctx) const override {
        ctx.printer().Print("\"{}\"", value_);
    }
    inline const std::string& value() const { return value_; }

private:
    std::string value_;
};

class CharExpression : public Expression {
public:
    CharExpression(char value, Span span) : Expression(span), value_(value) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline void Print(PrintableContext& ctx) const override {
        ctx.printer().Print("{}", value_);
    }
    inline char value() const { return value_; }

private:
    char value_;
};

class BoolExpression : public Expression {
public:
    BoolExpression(bool value, Span span) : Expression(span), value_(value) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline void Print(PrintableContext& ctx) const override {
        ctx.printer().Print("{}", value_);
    }
    inline bool value() const { return value_; }

private:
    bool value_;
};

class StructExpressionName {
public:
    StructExpressionName(std::string&& value, Span span)
        : value_(std::move(value)), span_(span) {}
    inline const std::string& value() const { return value_; }
    inline Span span() const { return span_; }

private:
    std::string value_;
    Span span_;
};

class StructExpressionInitName {
public:
    StructExpressionInitName(std::string&& value, Span span)
        : value_(std::move(value)), span_(span) {}
    inline const std::string& value() const { return value_; }
    inline Span span() const { return span_; }

private:
    std::string value_;
    Span span_;
};

class StructExpressionInit {
public:
    StructExpressionInit(StructExpressionInitName&& name,
                         std::unique_ptr<Expression>&& value)
        : name_(std::move(name)), value_(std::move(value)) {}
    inline const StructExpressionInitName& name() const { return name_; }
    inline const std::unique_ptr<Expression>& value() const { return value_; }
    inline Span span() const { return name_.span() + value_->span(); }

private:
    StructExpressionInitName name_;
    std::unique_ptr<Expression> value_;
};

class StructExpression : public Expression {
public:
    StructExpression(StructExpressionName&& name,
                     std::vector<StructExpressionInit>&& inits, Span span)
        : Expression(span), name_(std::move(name)), inits_(std::move(inits)) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    void Print(PrintableContext& ctx) const override;
    inline const StructExpressionName& name() const { return name_; }
    inline const std::vector<StructExpressionInit>& inits() const {
        return inits_;
    }

private:
    StructExpressionName name_;
    std::vector<StructExpressionInit> inits_;
};

class ArrayExpression : public Expression {
public:
    ArrayExpression(std::vector<std::unique_ptr<Expression>>&& inits, Span span)
        : Expression(span), inits_(std::move(inits)) {}
    inline void accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    void Print(PrintableContext& ctx) const override;
    inline const std::vector<std::unique_ptr<Expression>>& inits() const {
        return inits_;
    }

private:
    std::vector<std::unique_ptr<Expression>> inits_;
};

}  // namespace hir

}  // namespace mini

#endif  // MINI_HIR_EXPR_H_
