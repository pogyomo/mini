#ifndef MINI_CODEGEN_DECL_H_
#define MINI_CODEGEN_DECL_H_

#include "../hir/decl.h"
#include "context.h"

namespace mini {

class DeclCollect : public hir::DeclarationVisitor {
public:
    DeclCollect(CodeGenContext &ctx) : ctx_(ctx) {}
    void Visit(const hir::StructDeclaration &decl) override;
    void Visit(const hir::EnumDeclaration &decl) override;
    void Visit(const hir::FunctionDeclaration &decl) override;

private:
    CodeGenContext &ctx_;
};

class DeclCodeGen : public hir::DeclarationVisitor {
public:
    DeclCodeGen(CodeGenContext &ctx) : success_(false), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    void Visit(const hir::StructDeclaration &) override { success_ = true; }
    void Visit(const hir::EnumDeclaration &) override { success_ = true; }
    void Visit(const hir::FunctionDeclaration &decl) override;

private:
    bool success_;
    CodeGenContext &ctx_;
};

}  // namespace mini

#endif  // MINI_CODEGEN_DECL_H_
