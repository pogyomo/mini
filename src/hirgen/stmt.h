#ifndef MINI_HIRGEN_STMT_H_
#define MINI_HIRGEN_STMT_H_

#include <memory>
#include <vector>

#include "../ast/stmt.h"
#include "../hir/decl.h"
#include "../hir/stmt.h"
#include "context.h"

namespace mini {

class StmtHirGen : public ast::StatementVisitor {
public:
    StmtHirGen(HirGenContext &ctx)
        : success_(false), stmt_(nullptr), decls_(), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    std::unique_ptr<hir::Statement> &stmt() { return stmt_; }
    std::vector<hir::VariableDeclaration> &decls() { return decls_; }
    void Visit(const ast::ExpressionStatement &stmt) override;
    void Visit(const ast::ReturnStatement &stmt) override;
    void Visit(const ast::BreakStatement &stmt) override;
    void Visit(const ast::ContinueStatement &stmt) override;
    void Visit(const ast::WhileStatement &stmt) override;
    void Visit(const ast::IfStatement &stmt) override;
    void Visit(const ast::BlockStatement &stmt) override;

private:
    bool success_;
    std::unique_ptr<hir::Statement> stmt_;
    std::vector<hir::VariableDeclaration> decls_;
    HirGenContext &ctx_;
};

}  // namespace mini

#endif  // MINI_HIRGEN_STMT_H_
