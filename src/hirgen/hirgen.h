#ifndef MINI_HIRGEN_H_
#define MINI_HIRGEN_H_

#include <optional>
#include <string>

#include "../context.h"
#include "../hir/root.h"

namespace mini {

using HirGenResult = std::optional<hir::Root>;

HirGenResult HirGenFile(Context &ctx, const std::string &path);

}  // namespace mini

#endif  // MINI_HIRGEN_H_
