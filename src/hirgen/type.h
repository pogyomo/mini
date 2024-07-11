#ifndef MINI_HIRGEN_TYPE_H_
#define MINI_HIRGEN_TYPE_H_

#include "../ast/type.h"
#include "../eval.h"
#include "../hir/type.h"
#include "context.h"

namespace mini {

class TypeHirGen : public ast::TypeVisitor {
public:
    TypeHirGen(HirGenContext &ctx)
        : success_(false), type_(nullptr), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    const std::shared_ptr<hir::Type> &type() const { return type_; }
    void visit(const ast::BuiltinType &type) override {
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
    void visit(const ast::PointerType &type) override {
        TypeHirGen gen(ctx_);
        type.of()->accept(gen);
        if (!gen) return;

        type_ = std::make_shared<hir::PointerType>(gen.type_, type.span());
        success_ = true;
    }
    void visit(const ast::ArrayType &type) override {
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
    void visit(const ast::NameType &type) override {
        type_ = std::make_shared<hir::NameType>(std::string(type.name()),
                                                type.span());
        success_ = true;
    }

private:
    bool success_;
    std::shared_ptr<hir::Type> type_;
    HirGenContext &ctx_;
};

}  // namespace mini

#endif  // MINI_HIRGEN_TYPE_H_