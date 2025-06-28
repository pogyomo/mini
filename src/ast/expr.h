#ifndef MINI_AST_EXPR_H_
#define MINI_AST_EXPR_H_

#include <cstdint>
#include <vector>

#include "node.h"
#include "type.h"

namespace mini {

namespace ast {

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
class NullPtrExpression;
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
    virtual void Visit(const NullPtrExpression& expr) = 0;
    virtual void Visit(const StructExpression& expr) = 0;
    virtual void Visit(const ArrayExpression& expr) = 0;
};

class Expression : public Node {
public:
    virtual ~Expression() {}
    virtual void Accept(ExpressionVisitor& visitor) const = 0;
};

class UnaryExpression : public Expression {
public:
    class Op : public Node {
    public:
        enum Kind {
            Ref,    // "&"
            Deref,  // "*"
            Minus,  // "-"
            Inv,    // "~"
            Neg,    // "!"
        };
        Op(Kind kind, Span span) : kind_(kind), span_(span) {}
        inline Span span() const override { return span_; }
        inline Kind kind() const { return kind_; }

    private:
        const Kind kind_;
        const Span span_;
    };
    UnaryExpression(Op op, std::unique_ptr<Expression>&& expr)
        : op_(op), expr_(std::move(expr)) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override { return op_.span() + expr_->span(); }
    inline Op op() const { return op_; }
    inline const std::unique_ptr<Expression>& expr() const { return expr_; }

private:
    Op op_;
    std::unique_ptr<Expression> expr_;
};

class InfixExpression : public Expression {
public:
    class Op : public Node {
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
        inline Span span() const override { return span_; }
        inline Kind kind() const { return kind_; }

    private:
        Kind kind_;
        Span span_;
    };
    InfixExpression(Op op, std::unique_ptr<Expression>&& lhs,
                    std::unique_ptr<Expression>&& rhs)
        : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override { return lhs_->span() + rhs_->span(); }
    inline Op op() const { return op_; }
    inline const std::unique_ptr<Expression>& lhs() const { return lhs_; }
    inline const std::unique_ptr<Expression>& rhs() const { return rhs_; }

private:
    Op op_;
    std::unique_ptr<Expression> lhs_;
    std::unique_ptr<Expression> rhs_;
};

class IndexExpression : public Expression {
public:
    IndexExpression(std::unique_ptr<Expression>&& expr, LSquare lsquare,
                    std::unique_ptr<Expression>&& index, RSquare rsquare)
        : expr_(std::move(expr)),
          lsquare_(lsquare),
          index_(std::move(index)),
          rsquare_(rsquare) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override {
        return expr_->span() + rsquare_.span();
    }
    inline const std::unique_ptr<Expression>& expr() const { return expr_; }
    inline LSquare lsquare() const { return lsquare_; }
    inline const std::unique_ptr<Expression>& index() const { return index_; }
    inline RSquare rsquare() const { return rsquare_; }

private:
    std::unique_ptr<Expression> expr_;
    LSquare lsquare_;
    std::unique_ptr<Expression> index_;
    RSquare rsquare_;
};

class CallExpression : public Expression {
public:
    CallExpression(std::unique_ptr<Expression>&& func, LParen lparen,
                   std::vector<std::unique_ptr<Expression>>&& args,
                   RParen rparen)
        : func_(std::move(func)),
          lparen_(lparen),
          args_(std::move(args)),
          rparen_(rparen) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override { return func_->span() + rparen_.span(); }
    inline const std::unique_ptr<Expression>& func() const { return func_; }
    inline LParen lparen() const { return lparen_; }
    inline const std::vector<std::unique_ptr<Expression>>& args() const {
        return args_;
    }
    inline RParen rparen() const { return rparen_; }

private:
    std::unique_ptr<Expression> func_;
    LParen lparen_;
    std::vector<std::unique_ptr<Expression>> args_;
    RParen rparen_;
};

class AccessExpressionField : public Node {
public:
    AccessExpressionField(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

class AccessExpression : public Expression {
public:
    AccessExpression(std::unique_ptr<Expression>&& expr, Dot dot,
                     AccessExpressionField&& field)
        : expr_(std::move(expr)), dot_(dot), field_(std::move(field)) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override { return expr_->span() + field_.span(); }
    inline const std::unique_ptr<Expression>& expr() const { return expr_; }
    inline Dot dot() const { return dot_; }
    inline const AccessExpressionField& field() const { return field_; }

private:
    std::unique_ptr<Expression> expr_;
    Dot dot_;
    AccessExpressionField field_;
};

class CastExpression : public Expression {
public:
    CastExpression(std::unique_ptr<Expression>&& expr, As as_kw,
                   const std::shared_ptr<Type>& type)
        : expr_(std::move(expr)), as_kw_(as_kw), type_(type) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override { return expr_->span() + type_->span(); }
    inline const std::unique_ptr<Expression>& expr() const { return expr_; }
    inline As as_kw() const { return as_kw_; }
    inline const std::shared_ptr<Type>& type() const { return type_; }

private:
    std::unique_ptr<Expression> expr_;
    As as_kw_;
    std::shared_ptr<Type> type_;
};

class ESizeofExpression : public Expression {
public:
    ESizeofExpression(ESizeof esizeof_kw, std::unique_ptr<Expression>&& expr)
        : esizeof_kw_(esizeof_kw), expr_(std::move(expr)) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override {
        return esizeof_kw_.span() + expr_->span();
    }
    inline ESizeof esizeof_kw() const { return esizeof_kw_; }
    inline const std::unique_ptr<Expression>& expr() const { return expr_; }

private:
    ESizeof esizeof_kw_;
    std::unique_ptr<Expression> expr_;
};

class TSizeofExpression : public Expression {
public:
    TSizeofExpression(TSizeof tsizeof_kw, std::unique_ptr<Type>&& type)
        : tsizeof_kw_(tsizeof_kw), type_(std::move(type)) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override {
        return tsizeof_kw_.span() + type_->span();
    }
    inline TSizeof tsizeof_kw() const { return tsizeof_kw_; }
    inline const std::unique_ptr<Type>& type() const { return type_; }

private:
    TSizeof tsizeof_kw_;
    std::unique_ptr<Type> type_;
};

class EnumSelectExpressionDst : public Node {
public:
    EnumSelectExpressionDst(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

class EnumSelectExpressionSrc : public Node {
public:
    EnumSelectExpressionSrc(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

class EnumSelectExpression : public Expression {
public:
    EnumSelectExpression(EnumSelectExpressionDst&& dst, ColonColon colon_colon,
                         EnumSelectExpressionSrc&& src)
        : dst_(std::move(dst)),
          colon_colon_(colon_colon),
          src_(std::move(src)) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override { return dst_.span() + src_.span(); }
    inline const EnumSelectExpressionDst& dst() const { return dst_; }
    inline ColonColon colon_colon() const { return colon_colon_; }
    inline const EnumSelectExpressionSrc& src() const { return src_; }

private:
    EnumSelectExpressionDst dst_;
    ColonColon colon_colon_;
    EnumSelectExpressionSrc src_;
};

class VariableExpression : public Expression {
public:
    VariableExpression(std::string&& value, Span span)
        : value_(std::move(value)), span_(span) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override { return span_; }
    inline const std::string& value() const { return value_; }

private:
    std::string value_;
    Span span_;
};

class IntegerExpression : public Expression {
public:
    IntegerExpression(uint64_t value, Span span) : value_(value), span_(span) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override { return span_; }
    inline uint64_t value() const { return value_; }

private:
    uint64_t value_;
    Span span_;
};

class StringExpression : public Expression {
public:
    StringExpression(std::string&& value, Span span)
        : value_(std::move(value)), span_(span) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override { return span_; }
    inline const std::string& value() const { return value_; }

private:
    std::string value_;
    Span span_;
};

class CharExpression : public Expression {
public:
    CharExpression(char value, Span span) : value_(value), span_(span) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override { return span_; }
    inline char value() const { return value_; }

private:
    char value_;
    Span span_;
};

class BoolExpression : public Expression {
public:
    BoolExpression(bool value, Span span) : value_(value), span_(span) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override { return span_; }
    inline bool value() const { return value_; }

private:
    bool value_;
    Span span_;
};

class NullPtrExpression : public Expression {
public:
    NullPtrExpression(Span span) : span_(span) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override { return span_; }

private:
    Span span_;
};

class StructExpressionName : public Node {
public:
    StructExpressionName(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

class StructExpressionInitName : public Node {
public:
    StructExpressionInitName(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

class StructExpressionInit : public Node {
public:
    StructExpressionInit(StructExpressionInitName&& name, Colon colon,
                         std::unique_ptr<Expression>&& value)
        : name_(std::move(name)), colon_(colon), value_(std::move(value)) {}
    inline Span span() const override { return name_.span() + value_->span(); }
    inline const StructExpressionInitName& name() const { return name_; }
    inline Colon colon() const { return colon_; }
    inline const std::unique_ptr<Expression>& value() const { return value_; }

private:
    StructExpressionInitName name_;
    Colon colon_;
    std::unique_ptr<Expression> value_;
};

class StructExpression : public Expression {
public:
    StructExpression(StructExpressionName&& name, LCurly lcurly,
                     std::vector<StructExpressionInit>&& inits, RCurly rcurly)
        : name_(std::move(name)),
          lcurly_(lcurly),
          inits_(std::move(inits)),
          rcurly_(rcurly) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override { return name_.span() + rcurly_.span(); }
    inline const StructExpressionName& name() const { return name_; }
    inline LCurly lcurly() const { return lcurly_; }
    inline const std::vector<StructExpressionInit>& inits() const {
        return inits_;
    }
    inline RCurly rcurly() const { return rcurly_; }

private:
    StructExpressionName name_;
    LCurly lcurly_;
    std::vector<StructExpressionInit> inits_;
    RCurly rcurly_;
};

class ArrayExpression : public Expression {
public:
    ArrayExpression(LCurly lcurly,
                    std::vector<std::unique_ptr<Expression>>&& inits,
                    RCurly rcurly)
        : lcurly_(lcurly), inits_(std::move(inits)), rcurly_(rcurly) {}
    inline void Accept(ExpressionVisitor& visitor) const override {
        return visitor.Visit(*this);
    }
    inline Span span() const override {
        return lcurly_.span() + rcurly_.span();
    }
    inline LCurly lcurly() const { return lcurly_; }
    inline const std::vector<std::unique_ptr<Expression>>& inits() const {
        return inits_;
    }
    inline RCurly rcurly() const { return rcurly_; }

private:
    LCurly lcurly_;
    std::vector<std::unique_ptr<Expression>> inits_;
    RCurly rcurly_;
};

};  // namespace ast

};  // namespace mini

#endif  // MINI_AST_EXPR_H_
