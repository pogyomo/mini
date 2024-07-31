#ifndef MINI_HIRGEN_DECL_H_
#define MINI_HIRGEN_DECL_H_

#include <memory>

#include "../ast/decl.h"
#include "../hir/decl.h"
#include "context.h"

namespace mini {

class DeclVarReg : public ast::DeclarationVisitor {
public:
    DeclVarReg(HirGenContext &ctx) : ctx_(ctx) {}
    void Visit(const ast::FunctionDeclaration &decl) override;
    void Visit(const ast::StructDeclaration &decl) override;
    void Visit(const ast::EnumDeclaration &decl) override;
    void Visit(const ast::ImportDeclaration &) override {}

private:
    HirGenContext &ctx_;
};

class DeclHirGen : public ast::DeclarationVisitor {
public:
    DeclHirGen(HirGenContext &ctx)
        : success_(false), decl_(nullptr), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    std::unique_ptr<hir::Declaration> &decl() { return decl_; }
    void Visit(const ast::FunctionDeclaration &decl) override;
    void Visit(const ast::StructDeclaration &decl) override;
    void Visit(const ast::EnumDeclaration &decl) override;
    void Visit(const ast::ImportDeclaration &decl) override;

private:
    bool success_;
    std::unique_ptr<hir::Declaration> decl_;
    HirGenContext &ctx_;
};

}  // namespace mini

#endif  // MINI_HIRGEN_DECL_H_
