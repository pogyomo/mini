#ifndef MINI_HIROPT_HIROPT_H_
#define MINI_HIROPT_HIROPT_H_

#include "../context.h"
#include "../hir/root.h"

namespace mini {

namespace hiropt {

void OptimizeHirRoot(Context &ctx, hir::Root &root);

}  // namespace hiropt

}  // namespace mini

#endif  // MINI_HIROPT_HIROPT_H_
