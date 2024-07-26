#include "expr.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "../report.h"
#include "asm.h"
#include "context.h"
#include "fmt/format.h"
#include "type.h"

namespace mini {

static std::optional<std::string> IsVariable(
    const std::unique_ptr<hir::Expression> &expr) {
    class IsVariable : public hir::ExpressionVisitor {
    public:
        IsVariable() : success_(false) {}
        explicit operator bool() const { return success_; }
        const std::string &value() const { return value_; }
        void Visit(const hir::UnaryExpression &) override {}
        void Visit(const hir::InfixExpression &) override {}
        void Visit(const hir::IndexExpression &) override {}
        void Visit(const hir::CallExpression &) override {}
        void Visit(const hir::AccessExpression &) override {}
        void Visit(const hir::CastExpression &) override {}
        void Visit(const hir::ESizeofExpression &) override {}
        void Visit(const hir::TSizeofExpression &) override {}
        void Visit(const hir::EnumSelectExpression &) override {}
        void Visit(const hir::VariableExpression &expr) override {
            value_ = expr.value();
            success_ = true;
        }
        void Visit(const hir::IntegerExpression &) override {}
        void Visit(const hir::StringExpression &) override {}
        void Visit(const hir::CharExpression &) override {}
        void Visit(const hir::BoolExpression &) override {}
        void Visit(const hir::StructExpression &) override {}
        void Visit(const hir::ArrayExpression &) override {}

    private:
        bool success_;
        std::string value_;
    };

    IsVariable check;
    expr->Accept(check);
    if (check) {
        return check.value();
    } else {
        return std::nullopt;
    }
}

void ExprCodeGen::Visit(const hir::UnaryExpression &expr) {
    ReportInfo info(expr.span(), "not yet implemented", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprCodeGen::Visit(const hir::InfixExpression &expr) {
    ReportInfo info(expr.span(), "not yet implemented", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);

    if (expr.op().kind() == hir::InfixExpression::Op::Assign) {
    }
}

void ExprCodeGen::Visit(const hir::IndexExpression &expr) {
    ReportInfo info(expr.span(), "not yet implemented", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprCodeGen::Visit(const hir::CallExpression &expr) {
    auto var = IsVariable(expr.func());
    if (var && ctx_.func_info_table().Exists(var.value())) {
        auto &callee_info = ctx_.func_info_table().Query(var.value());

        if (callee_info.params().size() != expr.args().size()) {
            auto spec =
                fmt::format("expected {}, but got {}",
                            callee_info.params().size(), expr.args().size());
            ReportInfo info(expr.func()->span(),
                            "incorrect number of arguments", std::move(spec));
            Report(ctx_.ctx(), ReportLevel::Error, info);
        }

        auto &caller_table = ctx_.lvar_table();
        auto prev_size = caller_table.CalleeSize();

        // Change callee size if some arguments needs to store it to stack.
        caller_table.AlignCalleeSize(8);
        caller_table.AddCalleeSize(callee_info.lvar_table().CallerSize());
        caller_table.AlignCalleeSize(16);

        // Allocate stack for arguments.
        auto curr_size = caller_table.CalleeSize();
        if (curr_size != prev_size) {
            assert(curr_size > prev_size);
            ctx_.printer().PrintLn("  subq ${}, %rsp", curr_size - prev_size);
            caller_table.ChangeCallerSize(prev_size);
        }

        for (size_t i = 0; i < expr.args().size(); i++) {
            auto &arg = expr.args().at(i);
            ExprCodeGen gen(ctx_);
            arg->Accept(gen);
            if (!gen) return;

            auto &param_info = callee_info.lvar_table().Query(
                callee_info.params().at(i).first);
            if (param_info.ShouldInitializeWithReg()) {
                ctx_.printer().PrintLn("  movq %rax, {}",
                                       param_info.InitRegName());
            } else if (param_info.IsCallerAlloc()) {
                TypeSizeCalc calc(ctx_);
                param_info.type()->Accept(calc);
                if (!calc) return;

                IndexableAsmRegPtr src(Register::AX, 0);
                IndexableAsmRegPtr dst(Register::SP, param_info.Offset());
                CopyBytes(ctx_, src, dst, calc.size());
            } else {
                FatalError("unknown parameter");
            }
        }

        ctx_.printer().PrintLn("  callq {}", var.value());

        // Free allocated memory.
        if (curr_size != prev_size) {
            assert(curr_size > prev_size);
            ctx_.printer().PrintLn("  addq ${}, %rsp", curr_size - prev_size);
            caller_table.ChangeCallerSize(prev_size);
        }

        success_ = true;
    } else {
        ReportInfo info(expr.func()->span(), "not a callable", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
    }
}

void ExprCodeGen::Visit(const hir::AccessExpression &expr) {
    ExprCodeGen gen(ctx_);
    expr.expr()->Accept(gen);
    if (!gen) return;

    auto type = gen.inferred_;
    if (gen.inferred_->IsPointer()) {
        ctx_.printer().PrintLn("  movq (%rax), %rax");
        type = gen.inferred_->ToPointer()->of();
    }

    if (!type->IsName() ||
        !ctx_.struct_table().Exists(type->ToName()->value())) {
        ReportInfo info(expr.span(), "accessing to non-struct expression", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    auto &entry = ctx_.struct_table().Query(type->ToName()->value());

    if (!entry.Exists(expr.field().value())) {
        ReportInfo info(expr.field().span(), "no such field exists", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    auto field = entry.Query(expr.field().value());

    if (field.type()->IsBuiltin() || field.type()->IsPointer()) {
        TypeSizeCalc size(ctx_);
        field.type()->Accept(size);
        if (!size) return;

        if (size.size() == 1) {
            ctx_.printer().PrintLn("  movb {}(%rax), %al", field.Offset());
        } else if (size.size() == 2) {
            ctx_.printer().PrintLn("  movw {}(%rax), %ax", field.Offset());
        } else if (size.size() == 4) {
            ctx_.printer().PrintLn("  movl {}(%rax), %eax", field.Offset());
        } else if (size.size() == 8) {
            ctx_.printer().PrintLn("  movq {}(%rax), %rax", field.Offset());
        } else {
            FatalError("unreachable");
        }
    } else if (field.type()->IsArray() ||
               (field.type()->IsName() &&
                ctx_.struct_table().Exists(field.type()->ToName()->value()))) {
        ctx_.printer().PrintLn("  addq ${}, %rax", field.Offset());
    } else if (field.type()->IsName() &&
               ctx_.enum_table().Exists(field.type()->ToName()->value())) {
        ctx_.printer().PrintLn("  movq {}(%rax), %rax", field.Offset());
    } else {
        FatalError("unreaclable");
    }
}

void ExprCodeGen::Visit(const hir::CastExpression &expr) {
    ExprCodeGen gen(ctx_);
    expr.expr()->Accept(gen);
    if (!gen) return;

    if (gen.inferred_->IsPointer() && expr.cast_type()->IsPointer()) {
        inferred_ = expr.cast_type();
        success_ = true;
    } else if (gen.inferred_->IsBuiltin()) {
        if (expr.cast_type()->IsPointer()) {
            inferred_ = expr.cast_type();
            success_ = true;
        } else if (expr.cast_type()->IsBuiltin()) {
            inferred_ = expr.cast_type();
            success_ = true;
        } else {
            ReportInfo info(expr.span(), "cannot cast", "");
            Report(ctx_.ctx(), ReportLevel::Error, info);
        }
    } else {
        ReportInfo info(expr.span(), "cannot cast", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
    }
}

void ExprCodeGen::Visit(const hir::ESizeofExpression &expr) {
    ReportInfo info(expr.span(), "esizeof is not implemented", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
    return;
}

void ExprCodeGen::Visit(const hir::TSizeofExpression &expr) {
    TypeSizeCalc size(ctx_);
    expr.type()->Accept(size);
    if (!size) return;

    ctx_.printer().PrintLn("  movq ${}, %rax", size.size());
    inferred_ = std::make_shared<hir::BuiltinType>(hir::BuiltinType::USize,
                                                   expr.span());
    success_ = true;
}

void ExprCodeGen::Visit(const hir::EnumSelectExpression &expr) {
    if (!ctx_.enum_table().Exists(expr.src().value())) {
        ReportInfo info(expr.src().span(), "no such enum exists", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    auto &entry = ctx_.enum_table().Query(expr.src().value());
    if (!entry.Exists(expr.dst().value())) {
        ReportInfo info(expr.dst().span(), "no such enum variant exists", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    auto value = entry.Query(expr.dst().value());
    ctx_.printer().PrintLn("  movq ${}, %rax", value);

    inferred_ = std::make_shared<hir::NameType>(std::string(expr.src().value()),
                                                expr.span());
    success_ = true;
}

void ExprCodeGen::Visit(const hir::VariableExpression &expr) {
    if (!ctx_.lvar_table().Exists(expr.value())) {
        ReportInfo info(expr.span(), "no such variable exists", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    auto &entry = ctx_.lvar_table().Query(expr.value());
    ctx_.printer().PrintLn("  leaq {}, %rax", entry.AsmRepr());

    inferred_ = entry.type();
    success_ = true;
}

void ExprCodeGen::Visit(const hir::IntegerExpression &expr) {
    hir::BuiltinType::Kind kind;
    if (expr.value() <= UINT8_MAX) {
        kind = hir::BuiltinType::UInt8;
    } else if (expr.value() <= UINT16_MAX) {
        kind = hir::BuiltinType::UInt16;
    } else if (expr.value() <= UINT32_MAX) {
        kind = hir::BuiltinType::UInt32;
    } else {
        kind = hir::BuiltinType::UInt64;
    }
    ctx_.printer().PrintLn("  movq ${}, %rax", expr.value());
    inferred_ = std::make_shared<hir::BuiltinType>(kind, expr.span());
    success_ = true;
}

void ExprCodeGen::Visit(const hir::StringExpression &expr) {
    auto symbol = ctx_.string_table().QuerySymbol(expr.value());
    ctx_.printer().PrintLn("  movq ${}, %rax", symbol);

    auto of =
        std::make_shared<hir::BuiltinType>(hir::BuiltinType::Char, expr.span());
    inferred_ = std::make_shared<hir::ArrayType>(of, expr.value().size() + 1,
                                                 expr.span());
    success_ = true;
}

void ExprCodeGen::Visit(const hir::CharExpression &expr) {
    ctx_.printer().PrintLn("  movq ${}, %rax", (int)expr.value());
    inferred_ =
        std::make_shared<hir::BuiltinType>(hir::BuiltinType::Char, expr.span());
    success_ = true;
}

void ExprCodeGen::Visit(const hir::BoolExpression &expr) {
    ctx_.printer().PrintLn("  movq ${}, %rax", expr.value() ? 1 : 0);
    inferred_ =
        std::make_shared<hir::BuiltinType>(hir::BuiltinType::Bool, expr.span());
    success_ = true;
}

void ExprCodeGen::Visit(const hir::StructExpression &expr) {
    ReportInfo info(expr.span(),
                    "cannot use struct expression other than initialize", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprCodeGen::Visit(const hir::ArrayExpression &expr) {
    ReportInfo info(expr.span(),
                    "cannot use array expression other than initialize", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprRValGen::Visit(const hir::UnaryExpression &expr) {}

void ExprRValGen::Visit(const hir::InfixExpression &expr) {}

void ExprRValGen::Visit(const hir::IndexExpression &expr) {}

void ExprRValGen::Visit(const hir::CallExpression &expr) {}

void ExprRValGen::Visit(const hir::AccessExpression &expr) {}

void ExprRValGen::Visit(const hir::CastExpression &expr) {}

void ExprRValGen::Visit(const hir::ESizeofExpression &expr) {}

void ExprRValGen::Visit(const hir::TSizeofExpression &expr) {}

void ExprRValGen::Visit(const hir::EnumSelectExpression &expr) {}

void ExprRValGen::Visit(const hir::VariableExpression &expr) {
    if (!ctx_.lvar_table().Exists(expr.value())) {
        ReportInfo info(expr.span(), "no such variable exists", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    auto &entry = ctx_.lvar_table().Query(expr.value());

    ctx_.printer().PrintLn("  leaq -{}(%rbp), %rax", entry.Offset());
}

void ExprRValGen::Visit(const hir::IntegerExpression &expr) {
    ReportInfo info(expr.span(), "not a rvalue", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprRValGen::Visit(const hir::StringExpression &expr) {
    ReportInfo info(expr.span(), "not a rvalue", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprRValGen::Visit(const hir::CharExpression &expr) {
    ReportInfo info(expr.span(), "not a rvalue", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprRValGen::Visit(const hir::BoolExpression &expr) {
    ReportInfo info(expr.span(), "not a rvalue", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprRValGen::Visit(const hir::StructExpression &expr) {
    ReportInfo info(expr.span(), "not a rvalue", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprRValGen::Visit(const hir::ArrayExpression &expr) {
    ReportInfo info(expr.span(), "not a rvalue", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

}  // namespace mini
