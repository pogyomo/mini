#include "hiropt.h"

#include "unused.h"

namespace mini {

namespace hiropt {

void OptimizeHirRoot(Context &ctx, hir::Root &root) {
    RemoveUnusedVariable(ctx, root);
}

}  // namespace hiropt

}  // namespace mini
