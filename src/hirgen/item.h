#ifndef MINI_HIRGEN_ITEM_H_
#define MINI_HIRGEN_ITEM_H_

#include <memory>
#include <vector>

#include "../ast/stmt.h"
#include "../hir/decl.h"
#include "../hir/stmt.h"
#include "context.h"

namespace mini {

bool HirGenBlockItem(HirGenContext &ctx, const ast::BlockStatementItem &item,
                     std::vector<std::unique_ptr<hir::Statement>> &stmts,
                     std::vector<hir::VariableDeclaration> &decls);

}  // namespace mini

#endif  // MINI_HIRGEN_ITEM_H_
