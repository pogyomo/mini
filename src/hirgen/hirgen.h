#ifndef MINI_HIRGEN_H_
#define MINI_HIRGEN_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../context.h"
#include "../hir/decl.h"

namespace mini {

using HirgenResult =
    std::optional<std::vector<std::unique_ptr<hir::Declaration>>>;

HirgenResult HirgenFile(Context &ctx, const std::string &path);

}  // namespace mini

#endif  // MINI_HIRGEN_H_
