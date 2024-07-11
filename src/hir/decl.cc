#include "decl.h"

#include "fmt/format.h"

namespace mini {

namespace hir {

void StructDeclaration::print(PrintableContext &ctx) const {
    if (!fields_.empty()) {
        ctx.printer().shiftr();
        ctx.printer().println(fmt::format("struct {} {{", name_.value()));
        for (size_t i = 0; i < fields_.size(); i++) {
            auto &field = fields_.at(i);
            ctx.printer().print(fmt::format("{}: ", field.name().value()));
            field.type()->print(ctx);
            ctx.printer().print(",");
            if (i == fields_.size() - 1) {
                ctx.printer().shiftl();
            }
            ctx.printer().println("");
        }
        ctx.printer().print("}");
    } else {
        ctx.printer().print(fmt::format("struct {} {{}}", name_.value()));
    }
}

void EnumDeclaration::print(PrintableContext &ctx) const {
    if (!fields_.empty()) {
        ctx.printer().shiftr();
        ctx.printer().println(fmt::format("enum {} {{", name_.value()));
        for (size_t i = 0; i < fields_.size(); i++) {
            auto &field = fields_.at(i);
            if (i == fields_.size() - 1) {
                ctx.printer().shiftl();
            }
            ctx.printer().print(fmt::format("{} = {}", field.name().value(),
                                            field.value().value()));
            ctx.printer().print(",");
            if (i == fields_.size() - 1) {
                ctx.printer().shiftl();
            }
            ctx.printer().println("");
        }
        ctx.printer().print("}");
    } else {
        ctx.printer().print(fmt::format("enum {} {{}}", name_.value()));
    }
}

void FunctionDeclaration::print(PrintableContext &ctx) const {
    ctx.printer().print(fmt::format("function {}(", name_.value()));
    if (!params_.empty()) {
        auto param = params_.at(0);
        ctx.printer().print(fmt::format("{}: ", param.name().value()));
        param.type()->print(ctx);
        for (size_t i = 1; i < params_.size(); i++) {
            ctx.printer().print(", ");
            auto param = params_.at(i);
            ctx.printer().print(fmt::format("{}: ", param.name().value()));
            param.type()->print(ctx);
        }
    }
    if (!decls_.empty()) {
        ctx.printer().shiftr();
        ctx.printer().println(") {");
        for (size_t i = 0; i < decls_.size(); i++) {
            auto &decl = decls_.at(i);
            if (i == decls_.size() - 1) {
                ctx.printer().shiftl();
            }
            ctx.printer().print(fmt::format("let {}: ", decl.name().value()));
            decl.type()->print(ctx);
            ctx.printer().println(",");
        }
        ctx.printer().print("} ");
    } else {
        ctx.printer().print(") {} ");
    }
    body_.print(ctx);
}

}  // namespace hir

}  // namespace mini
