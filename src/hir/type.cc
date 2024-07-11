#include "type.h"

namespace mini {

namespace hir {

void BuiltinType::print(PrintableContext &ctx) const {
    if (kind_ == BuiltinType::Void)
        ctx.printer().print("void");
    else if (kind_ == BuiltinType::ISize)
        ctx.printer().print("isize");
    else if (kind_ == BuiltinType::Int8)
        ctx.printer().print("int8");
    else if (kind_ == BuiltinType::Int16)
        ctx.printer().print("int16");
    else if (kind_ == BuiltinType::Int32)
        ctx.printer().print("int32");
    else if (kind_ == BuiltinType::Int64)
        ctx.printer().print("int64");
    else if (kind_ == BuiltinType::USize)
        ctx.printer().print("usize");
    else if (kind_ == BuiltinType::UInt8)
        ctx.printer().print("uint8");
    else if (kind_ == BuiltinType::UInt16)
        ctx.printer().print("uint16");
    else if (kind_ == BuiltinType::UInt32)
        ctx.printer().print("uint32");
    else if (kind_ == BuiltinType::UInt64)
        ctx.printer().print("uint64");
    else if (kind_ == BuiltinType::Char)
        ctx.printer().print("char");
    else if (kind_ == BuiltinType::Bool)
        ctx.printer().print("bool");
    else
        fatal_error("unreachable");
}

void ArrayType::print(PrintableContext &ctx) const {
    if (size_) {
        ctx.printer().print("(");
        of_->print(ctx);
        ctx.printer().print(fmt::format(")[{}]", size_.value()));
    } else {
        ctx.printer().print("(");
        of_->print(ctx);
        ctx.printer().print(")[]");
    }
}

}  // namespace hir

}  // namespace mini
