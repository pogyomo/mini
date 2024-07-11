#include "decl.h"

#include "fmt/format.h"

namespace mini {

namespace hir {

void StructDeclaration::Print(PrintableContext &ctx) const {
    if (!fields_.empty()) {
        ctx.printer().ShiftR();
        ctx.printer().PrintLn(fmt::format("struct {} {{", name_.value()));
        for (size_t i = 0; i < fields_.size(); i++) {
            auto &field = fields_.at(i);
            ctx.printer().Print(fmt::format("{}: ", field.name().value()));
            field.type()->Print(ctx);
            ctx.printer().Print(",");
            if (i == fields_.size() - 1) {
                ctx.printer().ShiftL();
            }
            ctx.printer().PrintLn("");
        }
        ctx.printer().Print("}");
    } else {
        ctx.printer().Print(fmt::format("struct {} {{}}", name_.value()));
    }
}

void EnumDeclaration::Print(PrintableContext &ctx) const {
    if (!fields_.empty()) {
        ctx.printer().ShiftR();
        ctx.printer().PrintLn(fmt::format("enum {} {{", name_.value()));
        for (size_t i = 0; i < fields_.size(); i++) {
            auto &field = fields_.at(i);
            if (i == fields_.size() - 1) {
                ctx.printer().ShiftL();
            }
            ctx.printer().Print(fmt::format("{} = {}", field.name().value(),
                                            field.value().value()));
            ctx.printer().Print(",");
            if (i == fields_.size() - 1) {
                ctx.printer().ShiftL();
            }
            ctx.printer().PrintLn("");
        }
        ctx.printer().Print("}");
    } else {
        ctx.printer().Print(fmt::format("enum {} {{}}", name_.value()));
    }
}

void FunctionDeclaration::Print(PrintableContext &ctx) const {
    ctx.printer().Print(fmt::format("function {}(", name_.value()));
    if (!params_.empty()) {
        auto param = params_.at(0);
        ctx.printer().Print(fmt::format("{}: ", param.name().value()));
        param.type()->Print(ctx);
        for (size_t i = 1; i < params_.size(); i++) {
            ctx.printer().Print(", ");
            auto param = params_.at(i);
            ctx.printer().Print(fmt::format("{}: ", param.name().value()));
            param.type()->Print(ctx);
        }
    }
    if (!decls_.empty()) {
        ctx.printer().ShiftR();
        ctx.printer().PrintLn(") {");
        for (size_t i = 0; i < decls_.size(); i++) {
            auto &decl = decls_.at(i);
            if (i == decls_.size() - 1) {
                ctx.printer().ShiftL();
            }
            ctx.printer().Print(fmt::format("let {}: ", decl.name().value()));
            decl.type()->Print(ctx);
            ctx.printer().PrintLn(",");
        }
        ctx.printer().Print("} ");
    } else {
        ctx.printer().Print(") {} ");
    }
    body_.Print(ctx);
}

}  // namespace hir

}  // namespace mini
