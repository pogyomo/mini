#ifndef MINI_CFLOW_H_
#define MINI_CFLOW_H_

#include "context.h"
#include "hir/decl.h"

namespace mini {

class ControlFlowChecker : public hir::DeclarationVisitor {
public:
    ControlFlowChecker(Context &ctx) : success_(false), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    void Visit(const hir::StructDeclaration &decl) override;
    void Visit(const hir::EnumDeclaration &decl) override;
    void Visit(const hir::FunctionDeclaration &decl) override;

private:
    bool success_;
    Context &ctx_;
};

}  // namespace mini

#endif  // MINI_CFLOW_H_
