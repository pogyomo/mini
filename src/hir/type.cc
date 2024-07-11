#include "type.h"

namespace mini {

namespace hir {

void BuiltinType::Print(PrintableContext &ctx) const {
    if (kind_ == BuiltinType::Void)
        ctx.printer().Print("void");
    else if (kind_ == BuiltinType::ISize)
        ctx.printer().Print("isize");
    else if (kind_ == BuiltinType::Int8)
        ctx.printer().Print("int8");
    else if (kind_ == BuiltinType::Int16)
        ctx.printer().Print("int16");
    else if (kind_ == BuiltinType::Int32)
        ctx.printer().Print("int32");
    else if (kind_ == BuiltinType::Int64)
        ctx.printer().Print("int64");
    else if (kind_ == BuiltinType::USize)
        ctx.printer().Print("usize");
    else if (kind_ == BuiltinType::UInt8)
        ctx.printer().Print("uint8");
    else if (kind_ == BuiltinType::UInt16)
        ctx.printer().Print("uint16");
    else if (kind_ == BuiltinType::UInt32)
        ctx.printer().Print("uint32");
    else if (kind_ == BuiltinType::UInt64)
        ctx.printer().Print("uint64");
    else if (kind_ == BuiltinType::Char)
        ctx.printer().Print("char");
    else if (kind_ == BuiltinType::Bool)
        ctx.printer().Print("bool");
    else
        FatalError("unreachable");
}

void ArrayType::Print(PrintableContext &ctx) const {
    if (size_) {
        ctx.printer().Print("(");
        of_->Print(ctx);
        ctx.printer().Print(fmt::format(")[{}]", size_.value()));
    } else {
        ctx.printer().Print("(");
        of_->Print(ctx);
        ctx.printer().Print(")[]");
    }
}

}  // namespace hir

}  // namespace mini
