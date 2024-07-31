#include "root.h"

namespace mini {

namespace hir {

void Root::Print(PrintableContext &ctx) const {
    if (decls_.empty()) {
        auto size = string_table_.InnerRepr().size();
        for (const auto &s : string_table_.InnerRepr()) {
            size--;
            ctx.printer().Print("{} = \"{}\"", s.second, s.first);
            if (size == 0) {
                ctx.printer().PrintLn("");
            }
        }
    } else {
        if (!string_table_.InnerRepr().empty()) {
            for (const auto &s : string_table_.InnerRepr()) {
                ctx.printer().PrintLn("{} = \"{}\"", s.second, s.first);
            }
            ctx.printer().PrintLn("");
        }
        for (size_t i = 0; i < decls_.size(); i++) {
            if (i == decls_.size() - 1) {
                decls_.at(i)->Print(ctx);
            } else {
                decls_.at(i)->PrintLn(ctx);
                ctx.printer().PrintLn("");
            }
        }
    }
}

}  // namespace hir

}  // namespace mini
