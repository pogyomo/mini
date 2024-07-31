#include "item.h"

#include "../ast/stmt.h"
#include "../hir/decl.h"
#include "../hir/stmt.h"
#include "../report.h"
#include "context.h"
#include "expr.h"
#include "stmt.h"
#include "type.h"

namespace mini {

bool HirGenBlockItem(HirGenContext &ctx, const ast::BlockStatementItem &item,
                     std::vector<std::unique_ptr<hir::Statement>> &stmts,
                     std::vector<hir::VariableDeclaration> &decls) {
    if (item.IsStmt()) {
        StmtHirGen gen(ctx);
        item.stmt()->Accept(gen);
        if (!gen) return false;

        stmts.emplace_back(std::move(gen.stmt()));
        decls.insert(decls.end(), gen.decls().begin(), gen.decls().end());
    } else {
        for (const auto &body : item.decl().bodies()) {
            TypeHirGen gen_type(ctx);
            body.type()->Accept(gen_type);
            if (!gen_type) return false;

            if (ctx.translator().Translatable(body.name().name())) {
                ReportInfo info(body.name().span(), "variable already declared",
                                "");
                Report(ctx.ctx(), ReportLevel::Error, info);
                return false;
            }

            hir::VariableDeclarationName name(
                std::string(ctx.translator().RegName(body.name().name())),
                body.name().span());
            decls.emplace_back(gen_type.type(), std::move(name));

            if (body.init()) {
                ExprHirGen gen_expr(ctx);
                body.init()->expr()->Accept(gen_expr);
                if (!gen_expr) return false;

                if (gen_type.type()->IsArray() &&
                    !gen_type.type()->ToArray()->size() &&
                    gen_expr.array_size()) {
                    gen_type.type()->ToArray()->set_size(
                        gen_expr.array_size().value());
                }

                auto lhs = std::make_unique<hir::VariableExpression>(
                    std::string(ctx.translator().Translate(body.name().name())),
                    body.name().span());
                hir::InfixExpression::Op op(hir::InfixExpression::Op::Assign,
                                            body.init()->assign().span());
                auto expr = std::make_unique<hir::InfixExpression>(
                    std::move(lhs), op, std::move(gen_expr.expr()),
                    body.span());
                stmts.emplace_back(std::make_unique<hir::ExpressionStatement>(
                    std::move(expr), expr->span()));
            }
        }
    }
    return true;
}

}  // namespace mini
