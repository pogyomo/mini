#include "type.h"

#include "../eval.h"
#include "../report.h"

namespace mini {

void TypeHirGen::visit(const ast::BuiltinType &type) {
    hir::BuiltinType::Kind kind;
    if (type.kind() == ast::BuiltinType::USize)
        kind = hir::BuiltinType::USize;
    else if (type.kind() == ast::BuiltinType::UInt8)
        kind = hir::BuiltinType::UInt8;
    else if (type.kind() == ast::BuiltinType::UInt16)
        kind = hir::BuiltinType::UInt16;
    else if (type.kind() == ast::BuiltinType::UInt32)
        kind = hir::BuiltinType::UInt32;
    else if (type.kind() == ast::BuiltinType::UInt64)
        kind = hir::BuiltinType::UInt64;
    else if (type.kind() == ast::BuiltinType::ISize)
        kind = hir::BuiltinType::ISize;
    else if (type.kind() == ast::BuiltinType::Int8)
        kind = hir::BuiltinType::Int8;
    else if (type.kind() == ast::BuiltinType::Int16)
        kind = hir::BuiltinType::Int16;
    else if (type.kind() == ast::BuiltinType::Int32)
        kind = hir::BuiltinType::Int32;
    else if (type.kind() == ast::BuiltinType::Int64)
        kind = hir::BuiltinType::Int64;
    else if (type.kind() == ast::BuiltinType::Char)
        kind = hir::BuiltinType::Char;
    else if (type.kind() == ast::BuiltinType::Bool)
        kind = hir::BuiltinType::Bool;
    else
        fatal_error("unreachable");
    type_ = std::make_shared<hir::BuiltinType>(kind, type.span());
    success_ = true;
}

void TypeHirGen::visit(const ast::PointerType &type) {
    TypeHirGen gen(ctx_);
    type.of()->accept(gen);
    if (!gen) return;

    type_ = std::make_shared<hir::PointerType>(gen.type_, type.span());
    success_ = true;
}

void TypeHirGen::visit(const ast::ArrayType &type) {
    TypeHirGen gen(ctx_);
    type.of()->accept(gen);
    if (!gen) return;

    std::optional<uint64_t> size;
    if (type.size()) {
        ConstEval eval(ctx_.ctx());
        type.size().value()->accept(eval);
        if (!eval) return;
        size = eval.value();
    }

    type_ = std::make_shared<hir::ArrayType>(gen.type_, size, type.span());
    success_ = true;
}

void TypeHirGen::visit(const ast::NameType &type) {
    if (!ctx_.translator().translatable(type.name())) {
        ReportInfo info(type.span(), "no such name exists", "");
        report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    type_ = std::make_shared<hir::NameType>(
        std::string(ctx_.translator().translate(type.name())), type.span());
    success_ = true;
}

}  // namespace mini
