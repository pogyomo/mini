#ifndef MINI_AST_NODE_H_
#define MINI_AST_NODE_H_

#include "../span.h"

namespace ast {

#define GEN_NODE(name)                                      \
    class name : public Node {                              \
    public:                                                 \
        name(Span span) : span_(span) {}                    \
        inline Span span() const override { return span_; } \
                                                            \
    private:                                                \
        Span span_;                                         \
    };

class Node {
public:
    virtual ~Node() {}
    virtual Span span() const = 0;
};

GEN_NODE(Assign);
GEN_NODE(Dot);
GEN_NODE(LCurly);
GEN_NODE(LParen);
GEN_NODE(LSquare);
GEN_NODE(RCurly);
GEN_NODE(RParen);
GEN_NODE(RSquare);
GEN_NODE(Semicolon);
GEN_NODE(Star);
GEN_NODE(Arrow);
GEN_NODE(Colon);
GEN_NODE(ColonColon);

GEN_NODE(As);
GEN_NODE(Break);
GEN_NODE(Continue);
GEN_NODE(Else);
GEN_NODE(If);
GEN_NODE(Let);
GEN_NODE(Return);
GEN_NODE(While);
GEN_NODE(ESizeof);
GEN_NODE(TSizeof);
GEN_NODE(Function);
GEN_NODE(Struct);
GEN_NODE(Enum);

};  // namespace ast

#endif  // MINI_AST_NODE_H_
