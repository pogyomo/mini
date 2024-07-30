#ifndef MINI_CODEGEN_STMT_H_
#define MINI_CODEGEN_STMT_H_

#include "../hir/stmt.h"
#include "context.h"

namespace mini {

class StmtCodeGen : public hir::StatementVisitor {
public:
    StmtCodeGen(CodeGenContext &ctx) : success_(false), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    void Visit(const hir::ExpressionStatement &stmt) override;
    void Visit(const hir::ReturnStatement &stmt) override;
    void Visit(const hir::BreakStatement &stmt) override;
    void Visit(const hir::ContinueStatement &stmt) override;
    void Visit(const hir::WhileStatement &stmt) override;
    void Visit(const hir::IfStatement &stmt) override;
    void Visit(const hir::BlockStatement &stmt) override;

private:
    bool success_;
    CodeGenContext &ctx_;
};

}  // namespace mini

#endif  // MINI_CODEGEN_STMT_H_
