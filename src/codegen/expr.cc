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

static void AllocateAlignedStackMemory(CodeGenContext &ctx, uint64_t size,
                                       uint64_t align) {
    auto prev_size = ctx.lvar_table().CalleeSize();
    ctx.lvar_table().AddCalleeSize(size);
    ctx.lvar_table().AlignCalleeSize(align);
    auto diff = ctx.lvar_table().CalleeSize() - prev_size;
    if (diff) ctx.printer().PrintLn("    subq ${}, %rsp", diff);
}

void ExprRValGen::Visit(const hir::UnaryExpression &expr) {
    if (expr.op().kind() == hir::UnaryExpression::Op::Ref) {
        ExprLValGen gen(ctx_);
        expr.Accept(gen);
        if (!gen) return;

        success_ = true;
        inferred_ =
            std::make_shared<hir::PointerType>(gen.inferred(), expr.span());
    } else if (expr.op().kind() == hir::UnaryExpression::Op::Deref) {
        ReportInfo info(expr.span(), "not yet implemented", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
    } else if (expr.op().kind() == hir::UnaryExpression::Op::Minus) {
        ReportInfo info(expr.span(), "not yet implemented", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
    } else if (expr.op().kind() == hir::UnaryExpression::Op::Inv) {
        ReportInfo info(expr.span(), "not yet implemented", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
    } else if (expr.op().kind() == hir::UnaryExpression::Op::Neg) {
        ReportInfo info(expr.span(), "not yet implemented", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
    } else {
        FatalError("unreachable");
    }
}

void ExprRValGen::Visit(const hir::InfixExpression &expr) {
    if (expr.op().kind() == hir::InfixExpression::Op::Assign) {
        ExprLValGen gen_addr(ctx_);
        expr.lhs()->Accept(gen_addr);
        if (!gen_addr) return;

        // Offset to the address of lhs.
        const auto offset = ctx_.lvar_table().CalleeSize();

        std::optional<std::shared_ptr<hir::Type>> of;
        if (gen_addr.inferred()->IsPointer()) {
            of = gen_addr.inferred()->ToPointer()->of();
        } else if (gen_addr.inferred()->IsArray()) {
            of = gen_addr.inferred()->ToArray()->of();
        }

        ExprRValGen gen_rhs(ctx_, of);
        expr.rhs()->Accept(gen_rhs);
        if (!gen_rhs) return;

        // Convert rhs to type of lhs.
        if (!ImplicitlyConvertValueInStack(ctx_, gen_rhs.inferred_,
                                           gen_addr.inferred())) {
            return;
        }

        TypeSizeCalc size(ctx_);
        gen_addr.inferred()->Accept(size);
        if (!size) return;

        ctx_.printer().PrintLn("    movq -{}(%rbp), %rax", offset);

        IndexableAsmRegPtr src(Register::BP, -ctx_.lvar_table().CalleeSize());
        IndexableAsmRegPtr dst(Register::AX, 0);
        CopyBytes(ctx_, src, dst, size.size());

        success_ = true;
        inferred_ = gen_addr.inferred();
    } else {
        ReportInfo info(expr.span(), "not yet implemented", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
    }
}

void ExprRValGen::Visit(const hir::IndexExpression &expr) {
    ExprRValGen gen(ctx_);
    expr.expr()->Accept(gen);
    if (!gen) return;

    if (!gen.inferred_->IsArray() || gen.inferred_->IsPointer()) {
        ReportInfo info(expr.expr()->span(), "invalid indexing",
                        "not a array or pointer");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    std::shared_ptr<hir::Type> of;
    if (gen.inferred_->IsArray()) {
        of = gen.inferred_->ToArray()->of();
    } else {
        of = gen.inferred_->ToPointer()->of();
    }

    TypeSizeCalc of_size(ctx_);
    of->Accept(of_size);
    if (!of_size) return;

    ExprRValGen gen_index(ctx_);
    expr.index()->Accept(gen_index);
    if (!gen_index) return;

    // Convert index to usize.
    auto to = std::make_shared<hir::BuiltinType>(hir::BuiltinType::USize,
                                                 expr.index()->span());
    if (!ImplicitlyConvertValueInStack(ctx_, gen_index.inferred(), to)) return;

    // Pop index.
    ctx_.lvar_table().SubCalleeSize(8);
    ctx_.printer().PrintLn("    popq %rax");

    // Calculate address to element.
    ctx_.printer().PrintLn("    mulq ${}", of_size.size());
    ctx_.printer().PrintLn("    addq %rax, (%rsp)");

    // Offset to the address of element
    const auto offset = ctx_.lvar_table().CalleeSize();

    // Allocate memory for element.
    AllocateAlignedStackMemory(ctx_, of_size.size(), 8);

    // Store element to top of stack.
    IndexableAsmRegPtr src(Register::BP, -offset);
    IndexableAsmRegPtr dst(Register::BP, -ctx_.lvar_table().CalleeSize());
    CopyBytes(ctx_, src, dst, of_size.size());

    success_ = true;
    inferred_ = of;
}

void ExprRValGen::Visit(const hir::CallExpression &expr) {
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
            return;
        }

        auto &caller_table = ctx_.lvar_table();
        auto &callee_table = callee_info.lvar_table();

        // Allocate stack for arguments.
        auto size = callee_info.lvar_table().CallerSize();
        AllocateAlignedStackMemory(ctx_, size, 16);

        // Offset to the arguments block.
        const auto offset = caller_table.CalleeSize();

        // Prepare arguments.
        for (size_t i = 0; i < expr.args().size(); i++) {
            auto &arg = expr.args().at(i);
            auto &param_info = callee_info.lvar_table().Query(
                callee_info.params().at(i).first);

            std::optional<std::shared_ptr<hir::Type>> of;
            if (param_info.type()->IsPointer()) {
                of = param_info.type()->ToPointer()->of();
            } else if (param_info.type()->IsArray()) {
                of = param_info.type()->ToArray()->of();
            }

            caller_table.SaveCalleeSize();
            ExprRValGen gen(ctx_, of);
            arg->Accept(gen);
            if (!gen) return;

            // How many bytes `gen` allocated.
            const auto alloc_size = caller_table.CalleeSize() - offset;

            // Convert generated value to expected type.
            if (!ImplicitlyConvertValueInStack(ctx_, gen.inferred_,
                                               param_info.type())) {
                return;
            }

            if (param_info.ShouldInitializeWithReg()) {
                ctx_.printer().PrintLn("    movq (%rsp), {}",
                                       param_info.InitRegName());
            } else if (param_info.IsCallerAlloc()) {
                TypeSizeCalc calc(ctx_);
                param_info.type()->Accept(calc);
                if (!calc) return;

                // NOTE:
                // As param_info.Offset is relative position from the addres of
                // arguments block.
                // But currently `gen` allocated memory and rsp doesn't point to
                // the block, so I need to add `alloc_size` to
                // `param_info.Offset` to get proper address.
                IndexableAsmRegPtr src(Register::BP,
                                       -ctx_.lvar_table().CalleeSize());
                auto dst = param_info.CallerAsmRepr(alloc_size);
                CopyBytes(ctx_, src, dst, calc.size());
            } else {
                FatalError("unknown parameter");
            }

            // Free allocated memory.
            auto diff = caller_table.RestoreCalleeSize();
            if (diff) ctx_.printer().PrintLn("    addq ${}, %rsp", diff);
        }

        // If return value needs caller-allocated memory, move the address to
        // rdi.
        if (callee_table.Exists(callee_table.ret_name)) {
            auto &entry = callee_table.Query(callee_table.ret_name);
            ctx_.printer().PrintLn("    leaq {}, %rdi",
                                   entry.CallerAsmRepr(0).ToAsmRepr(0, 8));
        }

        ctx_.printer().PrintLn("    callq {}", var.value());

        // Ensure push returned value.
        caller_table.AddCalleeSize(8);
        ctx_.printer().PrintLn("    pushq %rax");

        inferred_ =
            ctx_.func_info_table().Query(ctx_.CurrFuncName()).ret_type();
        success_ = true;
    } else {
        ReportInfo info(expr.func()->span(), "not a callable", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
    }
}

void ExprRValGen::Visit(const hir::AccessExpression &expr) {
    ExprRValGen gen(ctx_);
    expr.expr()->Accept(gen);
    if (!gen) return;

    // If accessing to pointer, deref it.
    auto type = gen.inferred_;
    if (gen.inferred_->IsPointer()) {
        ctx_.printer().PrintLn("  leaq (%rsp), %rax");
        ctx_.printer().PrintLn("  movq %rax, (%rsp)");
        type = gen.inferred_->ToPointer()->of();
    }

    if (!type->IsName() ||
        !ctx_.struct_table().Exists(type->ToName()->value())) {
        ReportInfo info(expr.span(), "invalid access", "not a struct");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }
    auto &entry = ctx_.struct_table().Query(type->ToName()->value());

    if (!entry.Exists(expr.field().value())) {
        ReportInfo info(expr.field().span(), "invalid access", "not a struct");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }
    auto field = entry.Query(expr.field().value());

    // Change pointer in top of stack to point its field.
    ctx_.printer().PrintLn("    addq ${}, (%rsp)", field.Offset());

    // Offset to the pointer
    const auto offset = ctx_.lvar_table().CalleeSize();

    // Allocate memory for accessed field value.
    TypeSizeCalc field_size(ctx_);
    field.type()->Accept(field_size);
    if (!field_size) return;
    AllocateAlignedStackMemory(ctx_, field_size.size(), 8);

    // Store field to top of stack.
    IndexableAsmRegPtr src(Register::BP, -offset);
    IndexableAsmRegPtr dst(Register::BP, -ctx_.lvar_table().CalleeSize());
    CopyBytes(ctx_, src, dst, field_size.size());

    inferred_ = field.type();
    success_ = true;
}

void ExprRValGen::Visit(const hir::CastExpression &expr) {
    ExprRValGen gen(ctx_);
    expr.expr()->Accept(gen);
    if (!gen) return;

    // No convertion in assembly code.
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

    inferred_ = expr.cast_type();
    success_ = true;
}

void ExprRValGen::Visit(const hir::ESizeofExpression &expr) {
    ReportInfo info(expr.span(), "esizeof is not implemented", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
    return;
}

void ExprRValGen::Visit(const hir::TSizeofExpression &expr) {
    TypeSizeCalc size(ctx_);
    expr.type()->Accept(size);
    if (!size) return;

    ctx_.lvar_table().AddCalleeSize(8);
    ctx_.printer().PrintLn("    pushq ${}", size.size());

    inferred_ = std::make_shared<hir::BuiltinType>(hir::BuiltinType::USize,
                                                   expr.span());
    success_ = true;
}

void ExprRValGen::Visit(const hir::EnumSelectExpression &expr) {
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

    ctx_.lvar_table().AddCalleeSize(8);
    ctx_.printer().PrintLn("    pushq ${}", value);

    inferred_ = std::make_shared<hir::NameType>(std::string(expr.src().value()),
                                                expr.span());
    success_ = true;
}

void ExprRValGen::Visit(const hir::VariableExpression &expr) {
    if (!ctx_.lvar_table().Exists(expr.value())) {
        ReportInfo info(expr.span(), "no such variable exists", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    auto &entry = ctx_.lvar_table().Query(expr.value());

    TypeSizeCalc size(ctx_);
    entry.type()->Accept(size);
    if (!size) return;

    // Allocate memory for value.
    AllocateAlignedStackMemory(ctx_, size.size(), 8);

    // Push value the variable holds.
    auto src = entry.CalleeAsmRepr();
    IndexableAsmRegPtr dst(Register::SP, 0);
    CopyBytes(ctx_, src, dst, size.size());

    inferred_ = entry.type();
    success_ = true;
}

void ExprRValGen::Visit(const hir::IntegerExpression &expr) {
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

    ctx_.lvar_table().AddCalleeSize(8);
    ctx_.printer().PrintLn("    pushq ${}", expr.value());

    inferred_ = std::make_shared<hir::BuiltinType>(kind, expr.span());
    success_ = true;
}

void ExprRValGen::Visit(const hir::StringExpression &expr) {
    auto symbol = ctx_.string_table().QuerySymbol(expr.value());

    ctx_.lvar_table().AddCalleeSize(8);
    ctx_.printer().PrintLn("    pushq ${}", symbol);

    auto of =
        std::make_shared<hir::BuiltinType>(hir::BuiltinType::Char, expr.span());
    inferred_ = std::make_shared<hir::ArrayType>(of, expr.value().size() + 1,
                                                 expr.span());
    success_ = true;
}

void ExprRValGen::Visit(const hir::CharExpression &expr) {
    ctx_.lvar_table().AddCalleeSize(8);
    ctx_.printer().PrintLn("    pushq ${}", (int)expr.value());
    inferred_ =
        std::make_shared<hir::BuiltinType>(hir::BuiltinType::Char, expr.span());
    success_ = true;
}

void ExprRValGen::Visit(const hir::BoolExpression &expr) {
    ctx_.lvar_table().AddCalleeSize(8);
    ctx_.printer().PrintLn("    pushq ${}", expr.value() ? 1 : 0);
    inferred_ =
        std::make_shared<hir::BuiltinType>(hir::BuiltinType::Bool, expr.span());
    success_ = true;
}

void ExprRValGen::Visit(const hir::StructExpression &expr) {
    hir::NameType type(std::string(expr.name().value()), expr.span());

    if (!ctx_.struct_table().Exists(expr.name().value())) {
        ReportInfo info(expr.name().span(), "no such struct exists", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }
    auto &entry = ctx_.struct_table().Query(expr.name().value());

    TypeSizeCalc size(ctx_);
    type.Accept(size);
    if (!size) return;

    // Allocate memory for struct object.
    AllocateAlignedStackMemory(ctx_, size.size(), 8);

    const auto offset = ctx_.lvar_table().CalleeSize();
    for (size_t i = 0; i < expr.inits().size(); i++) {
        auto &init = expr.inits().at(i);

        if (!entry.Exists(init.name().value())) {
            ReportInfo info(expr.name().span(), "no such field exists", "");
            Report(ctx_.ctx(), ReportLevel::Error, info);
            return;
        }
        auto &field = entry.Query(init.name().value());

        ctx_.lvar_table().SaveCalleeSize();
        ExprRValGen gen(ctx_);
        expr.inits().at(i).value()->Accept(gen);
        if (!gen) return;

        // Convert generated value in stack so can be used to initialize field.
        if (!ImplicitlyConvertValueInStack(ctx_, gen.inferred_, field.type()))
            return;

        TypeSizeCalc field_size(ctx_);
        field.type()->Accept(field_size);
        if (!field_size) return;

        IndexableAsmRegPtr src(Register::BP, -ctx_.lvar_table().CalleeSize());
        IndexableAsmRegPtr dst(Register::BP, -offset);
        CopyBytes(ctx_, src, dst, field_size.size());

        // Free temporary generate value.
        auto diff = ctx_.lvar_table().RestoreCalleeSize();
        if (diff) ctx_.printer().PrintLn("    addq ${}, %rsp", diff);
    }

    // As array is manipulated by using its pointer, push pointer to array
    // object.
    ctx_.lvar_table().AddCalleeSize(8);
    ctx_.printer().PrintLn("    pushq %rsp");

    success_ = true;
    inferred_ = std::make_shared<hir::NameType>(type);
}

void ExprRValGen::Visit(const hir::ArrayExpression &expr) {
    if (!array_base_type_) {
        ReportInfo info(expr.span(), "failed to infer array type", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    TypeSizeCalc base_size(ctx_);
    array_base_type_.value()->Accept(base_size);
    if (!base_size) return;

    // Allocate memory for array object.
    AllocateAlignedStackMemory(ctx_, base_size.size() * expr.inits().size(), 8);

    const auto offset = ctx_.lvar_table().CalleeSize();
    for (size_t i = 0; i < expr.inits().size(); i++) {
        ctx_.lvar_table().SaveCalleeSize();
        ExprRValGen gen(ctx_);
        expr.inits().at(i)->Accept(gen);
        if (!gen) return;

        IndexableAsmRegPtr src(Register::BP, -ctx_.lvar_table().CalleeSize());
        IndexableAsmRegPtr dst(Register::BP, -offset + i * base_size.size());
        CopyBytes(ctx_, src, dst, base_size.size());

        // Free temporary generate value.
        auto diff = ctx_.lvar_table().RestoreCalleeSize();
        if (diff) ctx_.printer().PrintLn("    addq ${}, %rsp", diff);
    }

    // As struct is manipulated by using its pointer, push pointer to struct
    // object.
    ctx_.lvar_table().AddCalleeSize(8);
    ctx_.printer().PrintLn("    pushq %rsp");

    success_ = true;
    inferred_ = std::make_shared<hir::ArrayType>(
        array_base_type_.value(), expr.inits().size(), expr.span());
}

void ExprLValGen::Visit(const hir::UnaryExpression &expr) {
    if (expr.op().kind() == hir::UnaryExpression::Op::Deref) {
        ExprLValGen addr_gen(ctx_);
        expr.expr()->Accept(addr_gen);
        if (!addr_gen) return;

        if (!addr_gen.inferred_->IsPointer()) {
            ReportInfo info(expr.span(), "cannot deref non-pointer", "");
            Report(ctx_.ctx(), ReportLevel::Error, info);
            return;
        }

        ctx_.printer().PrintLn("    leaq (%rsp), %rax");
        ctx_.printer().PrintLn("    movq %rax, (%rsp)");

        success_ = true;
        inferred_ = addr_gen.inferred_->ToPointer()->of();
    } else {
        ReportInfo info(expr.span(), "invalid unary operator for lvalue", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
    }
}

void ExprLValGen::Visit(const hir::InfixExpression &expr) {
    if (expr.op().kind() == hir::InfixExpression::Op::Add ||
        expr.op().kind() == hir::InfixExpression::Op::Sub) {
        ExprLValGen lhs_addr_gen(ctx_);
        expr.lhs()->Accept(lhs_addr_gen);
        if (!lhs_addr_gen) return;

        if (!lhs_addr_gen.inferred_->IsPointer()) {
            ReportInfo info(expr.lhs()->span(),
                            "non-pointer cannot be lvalue of infix expression",
                            "");
            Report(ctx_.ctx(), ReportLevel::Error, info);
            return;
        }

        ExprRValGen rhs_gen(ctx_);
        expr.rhs()->Accept(rhs_gen);
        if (!rhs_gen) return;

        // Convert rhs to usize
        auto to = std::make_shared<hir::BuiltinType>(hir::BuiltinType::USize,
                                                     expr.rhs()->span());
        if (!ImplicitlyConvertValueInStack(ctx_, rhs_gen.inferred(), to))
            return;

        // Pop rhs
        ctx_.lvar_table().SubCalleeSize(8);
        ctx_.printer().PrintLn("    popq %rax");

        // Calculate added/subtracted pointer.
        if (expr.op().kind() == hir::InfixExpression::Op::Add) {
            ctx_.printer().PrintLn("    addq %rax, (%rsp)");
        } else {
            ctx_.printer().PrintLn("    subq %rax, (%rsp)");
        }

        success_ = true;
        inferred_ = lhs_addr_gen.inferred_;
    } else {
        ReportInfo info(expr.span(), "not a lvalue", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
    }
}

void ExprLValGen::Visit(const hir::IndexExpression &expr) {
    ExprLValGen gen_addr(ctx_);
    expr.expr()->Accept(gen_addr);
    if (!gen_addr) return;

    if (!gen_addr.inferred_->IsArray() || gen_addr.inferred_->IsPointer()) {
        ReportInfo info(expr.expr()->span(), "invalid indexing",
                        "not a array or pointer");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    std::shared_ptr<hir::Type> of;
    if (gen_addr.inferred_->IsArray()) {
        of = gen_addr.inferred_->ToArray()->of();
    } else {
        of = gen_addr.inferred_->ToPointer()->of();
    }

    TypeSizeCalc of_size(ctx_);
    of->Accept(of_size);
    if (!of_size) return;

    ExprRValGen gen_index(ctx_, std::nullopt);
    expr.index()->Accept(gen_index);
    if (!gen_index) return;

    // Convert index to usize.
    auto to = std::make_shared<hir::BuiltinType>(hir::BuiltinType::USize,
                                                 expr.index()->span());
    if (!ImplicitlyConvertValueInStack(ctx_, gen_index.inferred(), to)) return;

    // Pop index.
    ctx_.lvar_table().SubCalleeSize(8);
    ctx_.printer().PrintLn("    popq %rax");

    // Calculate address to element.
    ctx_.printer().PrintLn("    mulq ${}", of_size.size());
    ctx_.printer().PrintLn("    addq %rax, (%rsp)");
}

void ExprLValGen::Visit(const hir::CallExpression &expr) {
    ReportInfo info(expr.span(), "doesn't have address", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprLValGen::Visit(const hir::AccessExpression &expr) {
    ExprLValGen gen_addr(ctx_);
    expr.expr()->Accept(gen_addr);
    if (!gen_addr) return;

    // Deref pointer.
    auto type = gen_addr.inferred_;
    if (type->IsPointer()) {
        type = type->ToPointer()->of();
    }

    if (!gen_addr.inferred_->IsName()) {
        ReportInfo info(expr.expr()->span(), "invalid struct access",
                        "not a struct");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }
    auto &name = gen_addr.inferred_->ToName()->value();

    if (!ctx_.struct_table().Exists(name)) {
        FatalError("invalid struct inferred: {}", name);
    }
    auto &entry = ctx_.struct_table().Query(name);

    if (!entry.SizeAndOffsetCalculated()) {
        if (!CalculateStructSizeAndOffset(ctx_, name, expr.span())) {
            return;
        }
    }

    if (!entry.Exists(expr.field().value())) {
        ReportInfo info(expr.expr()->span(), "invalid struct access",
                        "no such field exists");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }
    auto &field = entry.Query(expr.field().value());

    ctx_.printer().PrintLn("    addq ${}, (%rsp)", field.Offset());

    success_ = true;
    inferred_ = field.type();
}

void ExprLValGen::Visit(const hir::CastExpression &expr) {
    ReportInfo info(expr.span(), "doesn't have address", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprLValGen::Visit(const hir::ESizeofExpression &expr) {
    ReportInfo info(expr.span(), "doesn't have address", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprLValGen::Visit(const hir::TSizeofExpression &expr) {
    ReportInfo info(expr.span(), "doesn't have address", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprLValGen::Visit(const hir::EnumSelectExpression &expr) {
    ReportInfo info(expr.span(), "doesn't have address", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprLValGen::Visit(const hir::VariableExpression &expr) {
    if (!ctx_.lvar_table().Exists(expr.value())) {
        ReportInfo info(expr.span(), "no such variable exists", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    auto &entry = ctx_.lvar_table().Query(expr.value());

    ctx_.lvar_table().AddCalleeSize(8);
    ctx_.printer().PrintLn("    leaq -{}(%rbp), %rax", entry.Offset());
    ctx_.printer().PrintLn("    pushq %rax");

    inferred_ = entry.type();
    success_ = true;
}

void ExprLValGen::Visit(const hir::IntegerExpression &expr) {
    ReportInfo info(expr.span(), "doesn't have address", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprLValGen::Visit(const hir::StringExpression &expr) {
    ReportInfo info(expr.span(), "doesn't have address", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprLValGen::Visit(const hir::CharExpression &expr) {
    ReportInfo info(expr.span(), "doesn't have address", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprLValGen::Visit(const hir::BoolExpression &expr) {
    ReportInfo info(expr.span(), "doesn't have address", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprLValGen::Visit(const hir::StructExpression &expr) {
    ReportInfo info(expr.span(), "doesn't have address", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprLValGen::Visit(const hir::ArrayExpression &expr) {
    ReportInfo info(expr.span(), "doesn't have address", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

bool ImplicitlyConvertValueInStack(CodeGenContext &ctx,
                                   const std::shared_ptr<hir::Type> &from,
                                   const std::shared_ptr<hir::Type> &to) {
    if (from->IsPointer()) {
        if (to->ToPointer()) {
            auto from_of = from->ToPointer()->of();
            auto to_of = to->ToPointer()->of();
            if (from_of->IsBuiltin() &&
                from_of->ToBuiltin()->kind() == hir::BuiltinType::Void) {
                return true;
            } else if (from_of == to_of) {
                return true;
            } else {
                goto failed;
            }
        } else {
            goto failed;
        }
    } else if (from->IsBuiltin()) {
        if (to->IsBuiltin()) {
            bool convertion_happen = false;
            auto from_kind = from->ToBuiltin()->kind();
            auto to_kind = to->ToBuiltin()->kind();
            if (from_kind == hir::BuiltinType::UInt8) {
                if (to_kind == hir::BuiltinType::UInt8) {
                    // no convertion
                } else if (to_kind == hir::BuiltinType::UInt16) {
                    ctx.printer().PrintLn("    movzbw (%rsp), %ax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::UInt32) {
                    ctx.printer().PrintLn("    movzbl (%rsp), %eax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::UInt64) {
                    ctx.printer().PrintLn("    movzbq (%rsp), %rax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::USize) {
                    ctx.printer().PrintLn("    movzbq (%rsp), %rax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int16) {
                    ctx.printer().PrintLn("    movsbw (%rsp), %ax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int32) {
                    ctx.printer().PrintLn("    movsbl (%rsp), %eax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int64) {
                    ctx.printer().PrintLn("    movsbq (%rsp), %rax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::ISize) {
                    ctx.printer().PrintLn("    movsbq (%rsp), %rax");
                    convertion_happen = true;
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::UInt16) {
                if (to_kind == hir::BuiltinType::UInt16) {
                    // no convertion
                } else if (to_kind == hir::BuiltinType::UInt32) {
                    ctx.printer().PrintLn("    movzwl (%rsp), %eax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::UInt64) {
                    ctx.printer().PrintLn("    movzwq (%rsp), %rax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::USize) {
                    ctx.printer().PrintLn("    movzwq (%rsp), %rax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int32) {
                    ctx.printer().PrintLn("    movswl (%rsp), %eax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int64) {
                    ctx.printer().PrintLn("    movswq (%rsp), %rax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::ISize) {
                    ctx.printer().PrintLn("    movswq (%rsp), %rax");
                    convertion_happen = true;
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::UInt32) {
                if (to_kind == hir::BuiltinType::UInt32) {
                    // no convertion
                } else if (to_kind == hir::BuiltinType::UInt64) {
                    // no convertion, as movzlq doesn't exists
                } else if (to_kind == hir::BuiltinType::USize) {
                    // no convertion, as movzlq doesn't exists
                } else if (to_kind == hir::BuiltinType::Int64) {
                    ctx.printer().PrintLn("    movslq (%rsp), %rax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::ISize) {
                    ctx.printer().PrintLn("    movslq (%rsp), %rax");
                    convertion_happen = true;
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::UInt64) {
                if (to_kind == hir::BuiltinType::UInt64) {
                    // no convertion
                } else if (to_kind == hir::BuiltinType::USize) {
                    // no convertion
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::USize) {
                if (to_kind == hir::BuiltinType::UInt64) {
                    // no convertion
                } else if (to_kind == hir::BuiltinType::USize) {
                    // no convertion
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::Int8) {
                if (to_kind == hir::BuiltinType::Int8) {
                    // no convertion
                } else if (to_kind == hir::BuiltinType::Int16) {
                    ctx.printer().PrintLn("    movsbw (%rsp), %ax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int32) {
                    ctx.printer().PrintLn("    movsbl (%rsp), %eax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int64) {
                    ctx.printer().PrintLn("    movsbq (%rsp), %rax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::ISize) {
                    ctx.printer().PrintLn("    movsbq (%rsp), %rax");
                    convertion_happen = true;
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::Int16) {
                if (to_kind == hir::BuiltinType::Int16) {
                    // no convertion
                } else if (to_kind == hir::BuiltinType::Int32) {
                    ctx.printer().PrintLn("    movswl (%rsp), %eax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int64) {
                    ctx.printer().PrintLn("    movswq (%rsp), %rax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::ISize) {
                    ctx.printer().PrintLn("    movswq (%rsp), %rax");
                    convertion_happen = true;
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::Int32) {
                if (to_kind == hir::BuiltinType::Int32) {
                    // no convertion
                } else if (to_kind == hir::BuiltinType::Int64) {
                    ctx.printer().PrintLn("    movslq (%rsp), %rax");
                    convertion_happen = true;
                } else if (to_kind == hir::BuiltinType::ISize) {
                    ctx.printer().PrintLn("    movslq (%rsp), %rax");
                    convertion_happen = true;
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::Int64) {
                if (to_kind == hir::BuiltinType::Int64) {
                    // no convertion
                } else if (to_kind == hir::BuiltinType::ISize) {
                    // no convertion
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::ISize) {
                if (to_kind == hir::BuiltinType::Int64) {
                    // no convertion
                } else if (to_kind == hir::BuiltinType::ISize) {
                    // no convertion
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::Void ||
                       from_kind == hir::BuiltinType::Char ||
                       from_kind == hir::BuiltinType::Bool) {
                if (from_kind == to_kind) {
                    return true;
                } else {
                    goto failed;
                }
            }
            if (convertion_happen)
                ctx.printer().PrintLn("    movq %rax, (%rsp)");
            return true;
        } else {
            goto failed;
        }
    } else if (from->IsArray()) {
        auto from_of = from->ToArray()->of();
        if (to->IsArray()) {
            auto to_of = to->ToArray()->of();
            return from_of == to_of;
        } else if (to->IsPointer()) {
            auto to_of = to->ToPointer()->of();
            return from_of == to_of;
        } else {
            goto failed;
        }
    } else {
        return from == to;
    }

failed:
    auto spec = fmt::format("cannot convert this {} to {} implicitly",
                            from->ToString(), to->ToString());
    ReportInfo info(from->span(), "implicit convertion failed",
                    std::move(spec));
    Report(ctx.ctx(), ReportLevel::Error, info);
    return false;
}

}  // namespace mini
