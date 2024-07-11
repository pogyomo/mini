#ifndef MINI_HIRGEN_H_
#define MINI_HIRGEN_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../context.h"
#include "../hir/decl.h"

namespace mini {

using HirGenResult =
    std::optional<std::vector<std::unique_ptr<hir::Declaration>>>;

HirGenResult HirGenFile(Context &ctx, const std::string &path);

}  // namespace mini

#endif  // MINI_HIRGEN_H_
