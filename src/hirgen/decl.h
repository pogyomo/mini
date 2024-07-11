#ifndef MINI_HIRGEN_DECL_H_
#define MINI_HIRGEN_DECL_H_

#include <iostream>
#include <memory>

#include "../ast/decl.h"
#include "../hir/decl.h"
#include "context.h"
#include "item.h"
#include "type.h"

namespace mini {

class DeclVarReg : public ast::DeclarationVisitor {
public:
    DeclVarReg(HirGenContext &ctx) : ctx_(ctx) {}
    void visit(const ast::FunctionDeclaration &decl) override {
        ctx_.translator().regvar(decl.name().name());
    }
    void visit(const ast::StructDeclaration &decl) override {
        ctx_.translator().regvar(decl.name().name());
    }
    void visit(const ast::EnumDeclaration &decl) override {
        ctx_.translator().regvar(decl.name().name());
    }

private:
    HirGenContext &ctx_;
};

class DeclHirGen : public ast::DeclarationVisitor {
public:
    DeclHirGen(HirGenContext &ctx)
        : success_(false), decl_(nullptr), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    std::unique_ptr<hir::Declaration> &decl() { return decl_; }
    void visit(const ast::FunctionDeclaration &decl) override {
        hir::FunctionDeclarationName name(
            std::string(ctx_.translator().translate(decl.name().name())),
            decl.name().span());

        ctx_.translator().enter_scope();

        std::vector<hir::FunctionDeclarationParam> params;
        for (const auto &param : decl.params()) {
            TypeHirGen gen(ctx_);
            param.type()->accept(gen);
            if (!gen) return;

            hir::FunctionDeclarationParamName name(
                std::string(ctx_.translator().regvar(param.name().name())),
                param.name().span());
            params.emplace_back(gen.type(), std::move(name), param.span());
        }

        std::shared_ptr<hir::Type> ret;
        if (decl.ret()) {
            TypeHirGen gen(ctx_);
            decl.ret()->type()->accept(gen);
            if (!gen) return;
            ret = gen.type();
        } else {
            ret = std::make_shared<hir::BuiltinType>(hir::BuiltinType::Void,
                                                     decl.span());
        }

        std::vector<std::unique_ptr<hir::Statement>> stmts;
        std::vector<hir::VariableDeclaration> decls;
        for (const auto &item : decl.body()->items()) {
            if (!hirgen_block_item(ctx_, item, stmts, decls)) return;
        }

        ctx_.translator().leave_scope();

        hir::BlockStatement body(std::move(stmts), decl.body()->span());
        decl_ = std::make_unique<hir::FunctionDeclaration>(
            std::move(name), std::move(params), ret, std::move(decls),
            std::move(body), decl.span());
        success_ = true;
    }
    void visit(const ast::StructDeclaration &decl) override {
        std::vector<hir::StructDeclarationField> fields;
        for (const auto &field : decl.fields()) {
            TypeHirGen gen(ctx_);
            field.type()->accept(gen);
            if (!gen) return;

            auto name = hir::StructDeclarationFieldName(
                std::string(field.name().name()), field.name().span());
            fields.emplace_back(gen.type(), std::move(name), field.span());
        }

        hir::StructDeclarationName name(
            std::string(ctx_.translator().translate(decl.name().name())),
            decl.name().span());
        decl_ = std::make_unique<hir::StructDeclaration>(
            std::move(name), std::move(fields), decl.span());
        success_ = true;
    }
    void visit(const ast::EnumDeclaration &decl) override {
        uint64_t value = 0;
        std::vector<hir::EnumDeclarationField> fields;
        for (const auto &field : decl.fields()) {
            if (field.init()) {
                ConstEval eval(ctx_.ctx());
                field.init()->value()->accept(eval);
                if (!eval) return;
                value = eval.value();
            }
            std::cout << value << std::endl;
            fields.emplace_back(
                hir::EnumDeclarationFieldName(std::string(field.name().name()),
                                              field.name().span()),
                hir::EnumDeclarationFieldValue(
                    value, field.init() ? field.init()->span() : field.span()));
            value++;
        }

        hir::EnumDeclarationName name(
            std::string(ctx_.translator().translate(decl.name().name())),
            decl.name().span());
        decl_ = std::make_unique<hir::EnumDeclaration>(
            std::move(name), std::move(fields), decl.span());
        success_ = true;
    }

private:
    bool success_;
    std::unique_ptr<hir::Declaration> decl_;
    HirGenContext &ctx_;
};

}  // namespace mini

#endif  // MINI_HIRGEN_DECL_H_
