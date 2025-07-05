#ifndef MINI_OPT_UNUSED_H_
#define MINI_OPT_UNUSED_H_

#include "../context.h"
#include "../hir/root.h"

namespace mini {

namespace hiropt {

void RemoveUnusedVariable(Context& ctx, hir::Root& root);

}  // namespace hiropt

}  // namespace mini

#endif  // MINI_OPT_UNUSED_H_
