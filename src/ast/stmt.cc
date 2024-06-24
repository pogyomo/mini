#include "stmt.h"

#include "expr.h"

inline Span ExpressionStatement::span() const {
    return expr_->span() + semicolon_.span();
}

inline Span VariableInit::span() const {
    return assign_.span() + expr_->span();
}
