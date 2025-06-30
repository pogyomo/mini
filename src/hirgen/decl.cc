#include "decl.h"

#include "../eval.h"
#include "../report.h"
#include "item.h"
#include "type.h"

namespace mini {

void DeclVarReg::Visit(const ast::FunctionDeclaration &decl) {
    ctx_.translator().RegNameRaw(decl.name().name());
}

void DeclVarReg::Visit(const ast::StructDeclaration &decl) {
    ctx_.translator().RegNameRaw(decl.name().name());
}

void DeclVarReg::Visit(const ast::EnumDeclaration &decl) {
    ctx_.translator().RegNameRaw(decl.name().name());
}

void DeclHirGen::Visit(const ast::FunctionDeclaration &decl) {
    hir::FunctionDeclarationName name(
        std::string(ctx_.translator().Translate(decl.name().name())),
        decl.name().span());

    ctx_.translator().EnterScope();
    ctx_.translator().EnterFunc();

    std::vector<hir::FunctionDeclarationParam> params;
    for (const auto &param : decl.params()) {
        TypeHirGen gen(ctx_);
        param.type()->Accept(gen);
        if (!gen) return;

        hir::FunctionDeclarationParamName name(
            std::string(ctx_.translator().RegName(param.name().name())),
            param.name().span());
        params.emplace_back(gen.type(), std::move(name), param.span());
    }

    std::optional<hir::FunctionDeclarationVariadic> variadic;
    if (decl.variadic()) variadic.emplace(decl.variadic()->span());

    std::shared_ptr<hir::Type> ret;
    if (decl.ret()) {
        TypeHirGen gen(ctx_);
        decl.ret()->type()->Accept(gen);
        if (!gen) return;
        ret = gen.type();
    } else {
        ret = std::make_shared<hir::BuiltinType>(hir::BuiltinType::Void,
                                                 decl.span());
    }

    std::vector<std::unique_ptr<hir::Statement>> stmts;
    std::vector<hir::VariableDeclaration> decls;
    if (decl.body().IsConcrete()) {
        for (const auto &item : decl.body().ToConcrete()->items()) {
            if (!HirGenBlockItem(ctx_, item, stmts, decls)) return;
        }
    }

    ctx_.translator().LeaveScope();

    if (decl.body().IsConcrete()) {
        hir::BlockStatement body(std::move(stmts), decl.body().span());
        decl_ = std::make_unique<hir::FunctionDeclaration>(
            std::move(name), std::move(params), variadic, ret, std::move(decls),
            std::move(body), decl.span());
    } else {
        decl_ = std::make_unique<hir::FunctionDeclaration>(
            std::move(name), std::move(params), variadic, ret, std::move(decls),
            std::nullopt, decl.span());
    }
    success_ = true;
}

void DeclHirGen::Visit(const ast::StructDeclaration &decl) {
    std::vector<hir::StructDeclarationField> fields;
    for (const auto &field : decl.fields()) {
        TypeHirGen gen(ctx_);
        field.type()->Accept(gen);
        if (!gen) return;

        auto name = hir::StructDeclarationFieldName(
            std::string(field.name().name()), field.name().span());
        fields.emplace_back(gen.type(), std::move(name), field.span());
    }

    hir::StructDeclarationName name(
        std::string(ctx_.translator().Translate(decl.name().name())),
        decl.name().span());
    decl_ = std::make_unique<hir::StructDeclaration>(
        std::move(name), std::move(fields), decl.span());
    success_ = true;
}

void DeclHirGen::Visit(const ast::EnumDeclaration &decl) {
    uint64_t value = 0;
    std::vector<hir::EnumDeclarationField> fields;
    for (const auto &field : decl.fields()) {
        if (field.init()) {
            ConstEval eval(ctx_.ctx());
            field.init()->value()->Accept(eval);
            if (!eval) return;
            value = eval.value();
        }
        fields.emplace_back(
            hir::EnumDeclarationFieldName(std::string(field.name().name()),
                                          field.name().span()),
            hir::EnumDeclarationFieldValue(
                value, field.init() ? field.init()->span() : field.span()));
        value++;
    }

    std::optional<std::shared_ptr<hir::Type>> base_type;
    if (decl.base_type()) {
        TypeHirGen gen(ctx_);
        decl.base_type()->type()->Accept(gen);
        if (!gen) return;
        base_type.emplace(gen.type());

        if (!gen.type()->IsBuiltin() || !gen.type()->ToBuiltin()->IsInteger()) {
            ReportInfo info(gen.type()->span(),
                            "non-integer type for enum base type", "");
            Report(ctx_.ctx(), ReportLevel::Error, info);
            return;
        }
    } else {
        std::shared_ptr<hir::Type> type = std::make_shared<hir::BuiltinType>(
            hir::BuiltinType::USize, decl.span());
        base_type.emplace(type);
    }

    hir::EnumDeclarationName name(
        std::string(ctx_.translator().Translate(decl.name().name())),
        decl.name().span());
    decl_ = std::make_unique<hir::EnumDeclaration>(
        std::move(name), base_type.value(), std::move(fields), decl.span());
    success_ = true;
}

}  // namespace mini
