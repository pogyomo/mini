#include "type.h"

#include "expr.h"

namespace ast {

// NOTE:
// Don't define constructor/destructor of `ArrayType` at header file, as it
// contains unique_ptr for opaque type `Expression`.
// refrences:
// * https://qiita.com/false-git@github/items/79bf1b6acc00dc43d173#comment-9071ab88afe79f2cf834
// * https://manu343726.github.io/2016-03-07-c++11-opaque-pointer-idiom/

ArrayType::ArrayType(LParen lparen, const std::shared_ptr<Type>& of,
                     RParen rparen, LSquare lsquare,
                     std::unique_ptr<Expression>&& size, RSquare rsquare)
    : lparen_(lparen),
      of_(of),
      rparen_(rparen),
      lsquare_(lsquare),
      size_(std::move(size)),
      rsquare_(rsquare) {}

ArrayType::~ArrayType() = default;

};  // namespace ast
