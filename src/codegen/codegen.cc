#include "codegen.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../ast/decl.h"
#include "../ast/expr.h"
#include "../ast/stmt.h"
#include "../ast/type.h"
#include "../context.h"
#include "../report.h"
#include "type.h"

class ExprTyassert : public ExpressionVisitor {};

class ExprCodegen : public ExpressionVisitor {
public:
    ExprCodegen(Context &ctx, std::ostream &os,
                const std::shared_ptr<Type> &expect_type)
        : success_(true),
          type_(nullptr),
          expect_type_(expect_type),
          ctx_(ctx),
          os_(os) {}
    bool success() const { return success_; }
    const std::shared_ptr<Type> type() const { return type_; }
    void visit(const UnaryExpression &expr) override {
        ExprCodegen gen(ctx_, os_, expect_type_);
        expr.expr()->accept(gen);
        if (!(success_ &= gen.success_)) return;

        if (expr.op().kind() == UnaryExpression::Op::Inv) {
        }
    }
    void visit(const InfixExpression &expr) override {
        ExprCodegen lhs_gen(ctx_, os_, expect_type_);
        expr.lhs()->accept(lhs_gen);
        if (!(success_ &= lhs_gen.success_)) return;

        ExprCodegen rhs_gen(ctx_, os_, expect_type_);
        expr.rhs()->accept(rhs_gen);
        if (!(success_ &= rhs_gen.success_)) return;

        if (expr.op().kind() == InfixExpression::Op::EQ) {
        } else if (expr.op().kind() == InfixExpression::Op::NE) {
        }
    }
    void visit(const IndexExpression &expr) override {
        fatal_error("not yet implemented");
    }
    void visit(const CallExpression &expr) override {
        fatal_error("not yet implemented");
    }
    void visit(const AccessExpression &expr) override {
        fatal_error("not yet implemented");
    }
    void visit(const CastExpression &expr) override {
        fatal_error("not yet implemented");
    }
    void visit(const ESizeofExpression &expr) override {
        fatal_error("not yet implemented");
    }
    void visit(const TSizeofExpression &expr) override {
        fatal_error("not yet implemented");
    }
    void visit(const EnumSelectExpression &expr) override {
        fatal_error("not yet implemented");
    }
    void visit(const VariableExpression &expr) override {
        class VariableCodegen : public SymbolTableEntryVisitor {
        public:
            VariableCodegen(Context &ctx, std::ostream &os)
                : success_(true), type_(nullptr), ctx_(ctx), os_(os) {}
            bool success() const { return success_; }
            const std::shared_ptr<Type> &type() const { return type_; }
            void visit(const VariableEntry &entry) override {
                os_ << "push-stack-base" << '\n';
                os_ << "push " << entry.offset() << '\n';
                os_ << "add" << '\n';
                os_ << "pop" << '\n';
                type_ = entry.type();
            }
            void visit(const StructEntry &entry) override {
                ReportInfo info(entry.span(), "not a variable", "");
                report(ctx_, ReportLevel::Error, info);
                success_ = false;
            }
            void visit(const EnumEntry &entry) override {
                ReportInfo info(entry.span(), "not a variable", "");
                report(ctx_, ReportLevel::Error, info);
                success_ = false;
            }
            void visit(const FunctionEntry &entry) override {
                ReportInfo info(entry.span(), "not a variable", "");
                report(ctx_, ReportLevel::Error, info);
                success_ = false;
            }

        private:
            bool success_;
            std::shared_ptr<Type> type_;
            Context &ctx_;
            std::ostream &os_;
        };

        try {
            auto &entry = ctx_.symbol_table()->query(expr.value());
            VariableCodegen gen(ctx_, os_);
            entry->accept(gen);
            if (!(success_ &= gen.success())) return;
            type_ = gen.type();
        } catch (std::out_of_range &e) {
            ReportInfo info(expr.span(), "no such variable exists", "");
            report(ctx_, ReportLevel::Error, info);
            success_ = false;
        }
    }
    void visit(const IntegerExpression &expr) override {
        os_ << "push-8 " << expr.value() << '\n';
        type_ = std::make_shared<UIntType>(expr.span());
    }
    void visit(const StringExpression &expr) override {
        for (char c : expr.value()) {
            os_ << "push-8 " << c << '\n';
        }
        type_ = std::make_shared<ArrayType>(
            LParen(expr.span()), std::make_shared<UIntType>(expr.span()),
            RParen(expr.span()), LSquare(expr.span()),
            std::make_unique<IntegerExpression>(expr.value().size(),
                                                expr.span()),
            RSquare(expr.span()));
    }
    void visit(const BoolExpression &expr) override {
        if (expr.value()) {
            os_ << "push-1 1" << '\n';
        } else {
            os_ << "push-1 0" << '\n';
        }
    }

private:
    bool success_;
    std::shared_ptr<Type> type_;
    std::shared_ptr<Type> expect_type_;
    Context &ctx_;
    std::ostream &os_;
};

class StmtCodegen : public StatementVisitor {
public:
    StmtCodegen(Context &ctx, std::ostream &os)
        : success_(true),
          ctx_(ctx),
          os_(os),
          loop_start_id_(-1),
          loop_end_id_(-1) {}
    StmtCodegen(Context &ctx, std::ostream &os, int loop_start_id,
                int loop_end_id)
        : success_(true),
          ctx_(ctx),
          os_(os),
          loop_start_id_(loop_start_id),
          loop_end_id_(loop_end_id) {}
    bool success() const { return success_; }
    void visit(const ExpressionStatement &stmt) override {
        ExprCodegen expr_gen(ctx_, os_,  // TODO: Add Good Type
                             std::make_shared<UIntType>(stmt.expr()->span()));
        stmt.expr()->accept(expr_gen);
        if (!(success_ &= expr_gen.success())) return;
        os_ << "pop" << '\n';
    }
    void visit(const ReturnStatement &stmt) override {
        os_ << "pop" << '\n';
        os_ << "ret" << '\n';
    }
    void visit(const BreakStatement &stmt) override {
        os_ << "goto " << "L" << loop_start_id_ << '\n';
    }
    void visit(const ContinueStatement &stmt) override {
        os_ << "goto " << "L" << loop_end_id_ << '\n';
    }
    void visit(const WhileStatement &stmt) override {
        int start_id = ctx_.fetch_unique_label_id();
        int end_id = ctx_.fetch_unique_label_id();

        os_ << "L" << start_id << ":" << '\n';

        ExprCodegen cond_gen(ctx_, os_,
                             std::make_shared<BoolType>(stmt.cond()->span()));
        stmt.cond()->accept(cond_gen);
        if (!(success_ &= cond_gen.success())) return;
        os_ << "pop" << '\n';
        os_ << "goto-non-zero " << "L" << end_id << '\n';

        StmtCodegen body_gen(ctx_, os_, start_id, end_id);
        stmt.body()->accept(body_gen);
        if (!(success_ &= body_gen.success())) return;
        os_ << "goto " << "L" << start_id << '\n';

        os_ << "L" << end_id << ":" << '\n';
    }
    void visit(const IfStatement &stmt) override {
        int else_label = ctx_.fetch_unique_label_id();
        int end_label = ctx_.fetch_unique_label_id();

        ExprCodegen cond_gen(ctx_, os_,
                             std::make_shared<BoolType>(stmt.cond()->span()));
        stmt.cond()->accept(cond_gen);
        if (!(success_ &= cond_gen.success())) return;

        os_ << "pop" << '\n';
        os_ << "goto-zero " << "L" << else_label << '\n';
        StmtCodegen body_gen(ctx_, os_, loop_start_id_, loop_end_id_);
        stmt.body()->accept(body_gen);
        if (!(success_ &= body_gen.success_)) return;
        os_ << "goto " << "L" << end_label << '\n';
        os_ << "L" << else_label << ":" << '\n';
        if (stmt.else_clause()) {
            StmtCodegen body_gen(ctx_, os_, loop_start_id_, loop_end_id_);
            stmt.else_clause()->body()->accept(body_gen);
            if (!(success_ &= body_gen.success_)) return;
        }
        os_ << "L" << end_label << ":" << '\n';
    }
    void visit(const BlockStatement &stmt) override {
        ctx_.dive_symbol_table();

        uint64_t offset = 0;
        for (const auto &decl : stmt.decls()) {
            for (const auto &body : decl.bodies()) {
                std::string name = body.name().name();
                auto entry = std::make_unique<VariableEntry>(
                    offset, body.type(), body.span());
                ctx_.symbol_table()->insert(std::move(name), std::move(entry));

                TypeSizeCalculator calc(ctx_, os_);
                body.type()->accept(calc);
                if (!(success_ &= calc.success())) return;
                offset += calc.size();

                if (body.init()) {
                    ExprCodegen init_gen(ctx_, os_, body.type());
                    body.init()->expr()->accept(init_gen);
                } else {
                    os_ << "reserve " << calc.size() << '\n';
                }
            }
        }

        for (const auto &stmt : stmt.stmts()) {
            StmtCodegen gen(ctx_, os_, loop_start_id_, loop_end_id_);
            stmt->accept(gen);
            if (!(gen.success_ &= gen.success_)) return;
        }

        ctx_.float_symbol_table();
    }

private:
    bool success_;
    Context &ctx_;
    std::ostream &os_;
    int loop_start_id_;
    int loop_end_id_;
};

class DeclCodegen : public DeclarationVisitor {
public:
    DeclCodegen(Context &ctx, std::ostream &os)
        : success_(true), ctx_(ctx), os_(os) {}
    bool success() const { return success_; }
    void visit(const FunctionDeclaration &decl) override {
        StmtCodegen body_gen(ctx_, os_);
        decl.body()->accept(body_gen);
        success_ &= body_gen.success();
    }
    void visit(const StructDeclaration &decl) override {}
    void visit(const EnumDeclaration &decl) override {}

private:
    bool success_;
    Context &ctx_;
    std::ostream &os_;
};

class DeclCollect : public DeclarationVisitor {
public:
    DeclCollect(Context &ctx, std::ostream &os) : ctx_(ctx), os_(os) {}
    void visit(const FunctionDeclaration &decl) override {
        std::optional<std::shared_ptr<Type>> ret;
        if (decl.ret()) ret = decl.ret()->type();
        std::vector<std::shared_ptr<Type>> params;
        for (const auto &param : decl.params()) {
            params.push_back(param.type());
        }
        ctx_.symbol_table()->insert(std::string(decl.name().name()),
                                    std::make_unique<FunctionEntry>(
                                        ret, std::move(params), decl.span()));
    }
    void visit(const StructDeclaration &decl) override {
        std::vector<StructField> fields;
        for (const auto &field : decl.fields()) {
            fields.emplace_back(std::shared_ptr<Type>(field.type()),
                                std::string(field.name().name()), field.span());
        }
        ctx_.symbol_table()->insert(
            std::string(decl.name().name()),
            std::make_unique<StructEntry>(std::move(fields), decl.span()));
    }
    void visit(const EnumDeclaration &decl) override {
        std::vector<EnumField> fields;
        for (size_t i = 0; i < decl.fields().size(); i++) {
            auto field = decl.fields().at(i);
            fields.emplace_back(std::string(field.name()), i, field.span());
        }
        ctx_.symbol_table()->insert(
            std::string(decl.name().name()),
            std::make_unique<EnumEntry>(std::move(fields), decl.span()));
    }

private:
    Context &ctx_;
    std::ostream &os_;
};

bool codegen(Context &ctx, std::ostream &os,
             const std::vector<std::unique_ptr<Declaration>> &decls) {
    bool success = true;
    for (const auto &decl : decls) {
        DeclCollect gen(ctx, os);
        decl->accept(gen);
    }
    for (const auto &decl : decls) {
        DeclCodegen gen(ctx, os);
        decl->accept(gen);
        success &= gen.success();
    }
    return success;
}
