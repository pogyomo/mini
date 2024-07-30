#ifndef MINI_PARSER_UTILS_H_
#define MINI_PARSER_UTILS_H_

#include "../context.h"
#include "stream.h"

namespace mini {

#define TRY(cond)            \
    if ((cond)) {            \
        return std::nullopt; \
    }

// Returns true if `ts` reaches to eos, and report it.
bool check_eos(Context &ctx, TokenStream &ts);

// Returns true if current token in `ts` is not ident, and report it.
bool check_ident(Context &ctx, TokenStream &ts);

// Returns true if current token in `ts` is not `kind`, and report it.
bool check_punct(Context &ctx, TokenStream &ts, PunctTokenKind kind);

// Returns true if current token in `ts` is not `kind`, and report it.
bool check_keyword(Context &ctx, TokenStream &ts, KeywordTokenKind kind);

}  // namespace mini

#endif  // MINI_PARSER_UTILS_H_
