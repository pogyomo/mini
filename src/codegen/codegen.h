#ifndef MINI_CODEGEN_H_
#define MINI_CODEGEN_H_

#include <memory>
#include <ostream>
#include <vector>

#include "../ast/decl.h"
#include "../context.h"

bool codegen(Context &ctx, std::ostream &os,
             const std::vector<std::unique_ptr<Declaration>> &decls);

#endif  // MINI_CODEGEN_H_
