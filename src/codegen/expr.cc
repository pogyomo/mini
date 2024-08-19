#include "expr.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../report.h"
#include "asm.h"
#include "context.h"
#include "fmt/format.h"
#include "type.h"

namespace mini {

static uint64_t RoundUp(uint64_t n, uint64_t t) {
    while (n % t) n++;
    return n;
}

static std::shared_ptr<hir::Type> ConvertArrayToPointer(
    const std::shared_ptr<hir::Type> &type) {
    if (!type->IsArray()) return type;
    return std::make_shared<hir::PointerType>(type->ToArray()->of(),
                                              type->span());
}

static std::shared_ptr<hir::Type> InferExprType(
    CodeGenContext &ctx, const std::unique_ptr<hir::Expression> &expr,
    std::optional<std::shared_ptr<hir::Type>> &array_base_type) {
    ctx.SuppressOutput();
    ctx.lvar_table().SaveCalleeSize();
    ExprRValGen gen(ctx, array_base_type);
    expr->Accept(gen);
    ctx.lvar_table().RestoreCalleeSize();
    ctx.ActivateOutput();
    return gen.inferred();
}

class ArgumentAssignmentTable {
public:
    class Entry {
    public:
        enum Kind {
            Reg,
            Stack,
        };
        Entry(const std::unique_ptr<hir::Expression> &arg,
              std::optional<std::shared_ptr<hir::Type>> &array_base_type,
              std::shared_ptr<hir::Type> &expect_type, Register reg)
            : kind_(Reg),
              arg_(arg),
              array_base_type_(array_base_type),
              expect_type_(expect_type),
              reg_(reg) {}
        Entry(const std::unique_ptr<hir::Expression> &arg,
              std::optional<std::shared_ptr<hir::Type>> &array_base_type,
              std::shared_ptr<hir::Type> &expect_type, uint64_t offset)
            : kind_(Stack),
              arg_(arg),
              array_base_type_(array_base_type),
              expect_type_(expect_type),
              offset_(offset) {}
        inline Kind kind() const { return kind_; }
        inline const std::unique_ptr<hir::Expression> &arg() const {
            return arg_;
        }
        inline const std::optional<std::shared_ptr<hir::Type>> &
        array_base_type() const {
            return array_base_type_;
        }
        inline const std::shared_ptr<hir::Type> &expect_type() const {
            return expect_type_;
        }
        inline const std::optional<Register> &reg() const { return reg_; }
        inline uint64_t offset() const { return offset_; }

    private:
        Kind kind_;
        const std::unique_ptr<hir::Expression> &arg_;
        std::optional<std::shared_ptr<hir::Type>> array_base_type_;
        std::shared_ptr<hir::Type> expect_type_;
        std::optional<Register> reg_;  // Used when kind_ == Reg.
        uint64_t offset_;              // Used when kind_ == Stack.
    };

    bool Build(CodeGenContext &ctx, bool big_ret,
               const std::vector<std::unique_ptr<hir::Expression>> &args,
               const FuncInfoTable::Entry::Params &params, bool has_variadic) {
        static Register regs[6] = {
            Register::DI, Register::SI, Register::DX,
            Register::CX, Register::R8, Register::R9,
        };

        assert(has_variadic || args.size() == params.size());

        uint8_t regnum = big_ret ? 1 : 0;
        uint64_t offset = 0;
        for (size_t i = 0; i < args.size(); i++) {
            auto &arg = args.at(i);

            std::optional<std::shared_ptr<hir::Type>> array_base_type;
            if (i < params.size() && params.at(i).second->IsArray()) {
                array_base_type.emplace(params.at(i).second->ToArray()->of());
            }
            auto inferred = InferExprType(ctx, arg, array_base_type);

            std::shared_ptr<hir::Type> expect_type =
                i < params.size() ? params.at(i).second
                                  : ConvertArrayToPointer(inferred);

            if (!ImplicitlyConvertValueInStack(ctx, arg->span(), inferred,
                                               expect_type)) {
                return false;
            }

            TypeSizeCalc size(ctx);
            expect_type->Accept(size);
            if (!size) return false;

            if (size.size() <= 8 && regnum < 6) {
                Entry entry(arg, array_base_type, expect_type, regs[regnum++]);
                entries_.push_back(entry);
            } else {
                Entry entry(arg, array_base_type, expect_type, offset);
                offset += RoundUp(size.size(), 8);
                entries_.push_back(entry);
            }
        }
        stack_size_ = offset;

        return true;
    }

    inline const std::vector<Entry> &Entries() const { return entries_; }
    inline uint64_t StackSize() const { return stack_size_; }

private:
    std::vector<Entry> entries_;
    uint64_t stack_size_;
};

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
        void Visit(const hir::NullPtrExpression &) override {}
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

// Returns true if the object which type is `type` should be passed by pointer
// when it was generated as rvalue.
static bool IsFatObject(CodeGenContext &ctx,
                        const std::shared_ptr<hir::Type> &type) {
    auto is_array = type->IsArray();
    auto is_struct =
        type->IsName() && ctx.struct_table().Exists(type->ToName()->value());
    return is_array || is_struct;
}

static void ReportErrorForUnaryExpression(
    CodeGenContext &ctx, const std::shared_ptr<hir::Type> &expr_type,
    Span op_span) {
    auto spec = fmt::format("cannot use it with {}", expr_type->ToString());
    ReportInfo info(op_span, "incorrect use of operator", std::move(spec));
    Report(ctx.ctx(), ReportLevel::Error, info);
}

static bool GenMinusExpr(CodeGenContext &ctx,
                         std::shared_ptr<hir::Type> &inferred,
                         const hir::UnaryExpression &expr) {
    ExprRValGen gen(ctx);
    expr.expr()->Accept(gen);
    if (!gen) return false;

    if (!gen.inferred()->IsBuiltin() ||
        !gen.inferred()->ToBuiltin()->IsInteger()) {
        ReportErrorForUnaryExpression(ctx, gen.inferred(), expr.op().span());
        return false;
    }

    auto builtin = gen.inferred()->ToBuiltin();
    TypeSizeCalc size(ctx);
    builtin->Accept(size);
    if (!size) return false;

    ctx.printer().PrintLn("    {} (%rsp)", AsmNeg(size.size()));

    // Change type to signed one.
    hir::BuiltinType::Kind kind;
    switch (builtin->kind()) {
        case hir::BuiltinType::UInt8:
            kind = hir::BuiltinType::Int8;
            break;
        case hir::BuiltinType::UInt16:
            kind = hir::BuiltinType::Int16;
            break;
        case hir::BuiltinType::UInt32:
            kind = hir::BuiltinType::Int32;
            break;
        case hir::BuiltinType::UInt64:
            kind = hir::BuiltinType::Int64;
            break;
        case hir::BuiltinType::USize:
            kind = hir::BuiltinType::ISize;
            break;
        default:
            kind = builtin->kind();
            break;
    }

    inferred = std::make_shared<hir::BuiltinType>(kind, expr.span());
    return true;
}

static bool GenInvExpr(CodeGenContext &ctx,
                       std::shared_ptr<hir::Type> &inferred,
                       const hir::UnaryExpression &expr) {
    ExprRValGen gen(ctx);
    expr.expr()->Accept(gen);
    if (!gen) return false;

    if (!gen.inferred()->IsBuiltin() ||
        !gen.inferred()->ToBuiltin()->IsInteger()) {
        ReportErrorForUnaryExpression(ctx, gen.inferred(), expr.op().span());
        return false;
    }

    auto builtin = gen.inferred()->ToBuiltin();
    TypeSizeCalc size(ctx);
    builtin->Accept(size);
    if (!size) return false;

    ctx.printer().PrintLn("    {} (%rsp)", AsmNot(size.size()));

    inferred = std::make_shared<hir::BuiltinType>(builtin->kind(), expr.span());
    return true;
}

static bool GenNegExpr(CodeGenContext &ctx,
                       std::shared_ptr<hir::Type> &inferred,
                       const hir::UnaryExpression &expr) {
    ExprRValGen gen(ctx);
    expr.expr()->Accept(gen);
    if (!gen) return false;

    if (!gen.inferred()->IsBuiltin() ||
        gen.inferred()->ToBuiltin()->kind() != hir::BuiltinType::Bool) {
        ReportErrorForUnaryExpression(ctx, gen.inferred(), expr.op().span());
        return false;
    }

    ctx.printer().PrintLn("    xorb $1, (%rsp)");

    inferred =
        std::make_shared<hir::BuiltinType>(hir::BuiltinType::Bool, expr.span());
    return true;
}

void ExprRValGen::Visit(const hir::UnaryExpression &expr) {
    if (expr.op().kind() == hir::UnaryExpression::Op::Ref) {
        ExprLValGen gen(ctx_);
        expr.expr()->Accept(gen);
        if (!gen) return;

        inferred_ =
            std::make_shared<hir::PointerType>(gen.inferred(), expr.span());
        success_ = true;
    } else if (expr.op().kind() == hir::UnaryExpression::Op::Deref) {
        ExprRValGen gen(ctx_);
        expr.expr()->Accept(gen);
        if (!gen) return;

        if (!gen.inferred_->IsPointer()) {
            ReportErrorForUnaryExpression(ctx_, gen.inferred_,
                                          expr.op().span());
            return;
        }
        auto &of = gen.inferred_->ToPointer()->of();

        // As we operate fat object through its address, we don't deref it.
        if (!IsFatObject(ctx_, of)) {
            // non-fat object can be stored to register.
            TypeSizeCalc size(ctx_);
            of->Accept(size);
            if (!size) return;
            assert(size.size() <= 8);

            // Copy address-pointing value.
            ctx_.printer().PrintLn("    movq (%rsp), %rax");
            ctx_.printer().PrintLn("    movq (%rax), %rax");
            ctx_.printer().PrintLn("    movq %rax, (%rsp)");
        }

        inferred_ = of;
        success_ = true;
    } else if (expr.op().kind() == hir::UnaryExpression::Op::Minus) {
        success_ = GenMinusExpr(ctx_, inferred_, expr);
    } else if (expr.op().kind() == hir::UnaryExpression::Op::Inv) {
        success_ = GenInvExpr(ctx_, inferred_, expr);
    } else if (expr.op().kind() == hir::UnaryExpression::Op::Neg) {
        success_ = GenNegExpr(ctx_, inferred_, expr);
    } else {
        FatalError("unreachable");
    }
}

// NOTE:
// In below functions for infix expression, I **always** free memory allocated
// by rhs after pop value, as we expected lhs and rhs is continueous and two
// value can be obtained by only tow pop.
// But some expression may allocate memory more than 8 bytes, and it breaks the
// assumption. So I free memory so that top of stack to be the value that lhs
// generated.

static void ReportErrorForInfixExpression(
    CodeGenContext &ctx, const std::shared_ptr<hir::Type> &lhs_type,
    const std::shared_ptr<hir::Type> &rhs_type, Span op_span) {
    auto spec = fmt::format("cannot use it with {} and {}",
                            lhs_type->ToString(), rhs_type->ToString());
    ReportInfo info(op_span, "incorrect use of operator", std::move(spec));
    Report(ctx.ctx(), ReportLevel::Error, info);
}

static bool GenAssignExpr(CodeGenContext &ctx,
                          std::shared_ptr<hir::Type> &inferred,
                          const std::unique_ptr<hir::Expression> &lhs,
                          const std::unique_ptr<hir::Expression> &rhs) {
    ExprLValGen gen_addr(ctx);
    lhs->Accept(gen_addr);
    if (!gen_addr) return false;

    // Offset to the address of lhs.
    const auto offset = ctx.lvar_table().CalleeSize();

    std::optional<std::shared_ptr<hir::Type>> of;
    if (gen_addr.inferred()->IsPointer()) {
        of = gen_addr.inferred()->ToPointer()->of();
    } else if (gen_addr.inferred()->IsArray()) {
        of = gen_addr.inferred()->ToArray()->of();
    }

    ExprRValGen gen_rhs(ctx, of);
    rhs->Accept(gen_rhs);
    if (!gen_rhs) return false;

    // Convert rhs to type of lhs.
    if (!ImplicitlyConvertValueInStack(ctx, rhs->span(), gen_rhs.inferred(),
                                       gen_addr.inferred())) {
        return false;
    }

    TypeSizeCalc size(ctx);
    gen_addr.inferred()->Accept(size);
    if (!size) return false;

    // -offset(%rbp) contains address to the variable, so copy the address.
    ctx.printer().PrintLn("    movq -{}(%rbp), %rax", offset);

    // If the object is fat, then copy the address in stack.
    // Otherwise copy address to the value.
    if (IsFatObject(ctx, gen_addr.inferred())) {
        ctx.printer().PrintLn("    movq -{}(%rbp), %rbx",
                              ctx.lvar_table().CalleeSize());

        // Copy rhs to lhs.
        IndexableAsmRegPtr src(Register::BX, 0);
        IndexableAsmRegPtr dst(Register::AX, 0);
        CopyBytes(ctx, src, dst, size.size());
    } else {
        assert(size.size() <= 8);
        ctx.printer().PrintLn("    movq -{}(%rbp), %rbx",
                              ctx.lvar_table().CalleeSize());
        ctx.printer().PrintLn("    movq %rbx, (%rax)");
    }

    inferred = gen_addr.inferred();
    return true;
}

static bool GenAdditiveExpr(CodeGenContext &ctx,
                            std::shared_ptr<hir::Type> &inferred,
                            const hir::InfixExpression &expr) {
    auto is_add = expr.op().kind() == hir::InfixExpression::Op::Add;
    auto &lhs = expr.lhs();
    auto &rhs = expr.rhs();

    ExprRValGen gen_lhs(ctx);
    lhs->Accept(gen_lhs);
    if (!gen_lhs) return false;

    ctx.lvar_table().SaveCalleeSize();

    ExprRValGen gen_rhs(ctx);
    rhs->Accept(gen_rhs);
    if (!gen_rhs) return false;

    if (gen_lhs.inferred()->IsPointer()) {
        auto to = std::make_shared<hir::BuiltinType>(hir::BuiltinType::USize,
                                                     lhs->span());
        if (!ImplicitlyConvertValueInStack(ctx, rhs->span(), gen_rhs.inferred(),
                                           to)) {
            return false;
        }

        TypeSizeCalc size(ctx);
        gen_lhs.inferred()->ToPointer()->of()->Accept(size);
        if (!size) return false;

        // Calculate how much to add/sub to pointer.
        ctx.lvar_table().SubCalleeSize(8);
        ctx.printer().PrintLn("    popq %rax");
        ctx.printer().PrintLn("    movq ${}, %rbx", size.size());
        ctx.printer().PrintLn("    mulq %rbx");

        // Free memory allocated by rhs.
        auto diff = ctx.lvar_table().RestoreCalleeSize();
        if (diff) ctx.printer().PrintLn("    addq ${}, %rsp", diff);

        // Increment/decrement pointer
        if (is_add) {
            ctx.printer().PrintLn("   addq %rax, (%rsp)");
        } else {
            ctx.printer().PrintLn("   subq %rax, (%rsp)");
        }

        inferred = gen_lhs.inferred();
        return true;
    } else if (gen_lhs.inferred()->IsBuiltin() &&
               gen_rhs.inferred()->IsBuiltin()) {
        auto merged =
            ImplicitlyMergeTwoType(ctx, gen_lhs.inferred(), gen_rhs.inferred());
        if (!merged || !merged.value()->IsBuiltin() ||
            !merged.value()->ToBuiltin()->IsInteger()) {
            ReportErrorForInfixExpression(ctx, gen_lhs.inferred(),
                                          gen_rhs.inferred(), expr.op().span());
            return false;
        }

        auto builtin = merged.value()->ToBuiltin();
        TypeSizeCalc size(ctx);
        builtin->Accept(size);
        if (!size) return false;

        // Convert rhs to proper type, then pop rhs from stack
        if (!ImplicitlyConvertValueInStack(ctx, rhs->span(), gen_rhs.inferred(),
                                           merged.value())) {
            return false;
        }
        ctx.lvar_table().SubCalleeSize(8);
        ctx.printer().PrintLn("    popq %rbx");

        // Free memory allocated by rhs.
        auto diff = ctx.lvar_table().RestoreCalleeSize();
        if (diff) ctx.printer().PrintLn("    addq ${}, %rsp", diff);

        // Convert lhs to proper type.
        if (!ImplicitlyConvertValueInStack(ctx, lhs->span(), gen_lhs.inferred(),
                                           merged.value())) {
            return false;
        }

        if (is_add) {
            ctx.printer().PrintLn(
                "    {} {}, (%rsp)", AsmAdd(size.size()),
                Register(Register::BX).ToNameBySize(size.size()));
        } else {
            ctx.printer().PrintLn(
                "    {} {}, (%rsp)", AsmSub(size.size()),
                Register(Register::BX).ToNameBySize(size.size()));
        }
        inferred = merged.value();
        return true;
    } else {
        ReportErrorForInfixExpression(ctx, gen_lhs.inferred(),
                                      gen_rhs.inferred(), expr.op().span());
        return false;
    }
}

static bool GenMultiplicativeExpr(CodeGenContext &ctx,
                                  std::shared_ptr<hir::Type> &inferred,
                                  const hir::InfixExpression &expr) {
    auto &lhs = expr.lhs();
    auto &rhs = expr.rhs();

    ExprRValGen gen_lhs(ctx);
    lhs->Accept(gen_lhs);
    if (!gen_lhs) return false;

    ctx.lvar_table().SaveCalleeSize();

    ExprRValGen gen_rhs(ctx);
    rhs->Accept(gen_rhs);
    if (!gen_rhs) return false;

    if (gen_lhs.inferred()->IsBuiltin() && gen_rhs.inferred()->IsBuiltin()) {
        auto merged =
            ImplicitlyMergeTwoType(ctx, gen_lhs.inferred(), gen_rhs.inferred());
        if (!merged || !merged.value()->IsBuiltin() ||
            !merged.value()->ToBuiltin()->IsInteger()) {
            ReportErrorForInfixExpression(ctx, gen_lhs.inferred(),
                                          gen_rhs.inferred(), expr.op().span());
            return false;
        }

        auto builtin = merged.value()->ToBuiltin();
        TypeSizeCalc size(ctx);
        builtin->Accept(size);
        if (!size) return false;

        // Convert rhs to proper type, then pop it from stack
        if (!ImplicitlyConvertValueInStack(ctx, rhs->span(), gen_rhs.inferred(),
                                           merged.value())) {
            return false;
        }
        ctx.lvar_table().SubCalleeSize(8);
        ctx.printer().PrintLn("    popq %rbx");

        // Free memory allocated by rhs.
        auto diff = ctx.lvar_table().RestoreCalleeSize();
        if (diff) ctx.printer().PrintLn("    addq ${}, %rsp", diff);

        // Convert lhs to proper type, then pop it from stack.
        if (!ImplicitlyConvertValueInStack(ctx, lhs->span(), gen_lhs.inferred(),
                                           merged.value())) {
            return false;
        }
        ctx.lvar_table().SubCalleeSize(8);
        ctx.printer().PrintLn("    popq %rax");

        if (expr.op().kind() == hir::InfixExpression::Op::Mul) {
            // rax = rax * rbx
            ctx.printer().PrintLn(
                "    {} {}", AsmMul(builtin->IsSigned(), size.size()),
                Register(Register::BX).ToNameBySize(size.size()));

            // Push result.
            ctx.lvar_table().AddCalleeSize(8);
            ctx.printer().PrintLn("    pushq %rax");
        } else {
            // Extend rax to rdx, as div uses rax and rdx
            if (size.size() == 2) {
                ctx.printer().PrintLn("    cwd");
            } else if (size.size() == 4) {
                ctx.printer().PrintLn("    cdq");
            } else if (size.size() == 8) {
                ctx.printer().PrintLn("    cqo");
            }

            // rax, rdx (al, ah) = rax / rbx
            ctx.printer().PrintLn(
                "    {} {}", AsmDiv(builtin->IsSigned(), size.size()),
                Register(Register::BX).ToNameBySize(size.size()));

            // Push result.
            ctx.lvar_table().AddCalleeSize(8);
            if (size.size() == 1) {
                if (expr.op().kind() == hir::InfixExpression::Op::Div) {
                    ctx.printer().PrintLn("    pushq %rax");
                } else {
                    // 1-byte division store modulus to ah, so move it to al so
                    // that the value can be accessed by normal way.
                    ctx.printer().PrintLn("    movb %ah, %al");
                    ctx.printer().PrintLn("    pushq %rax");
                }
            } else {
                if (expr.op().kind() == hir::InfixExpression::Op::Div) {
                    ctx.printer().PrintLn("    pushq %rax");
                } else {
                    ctx.printer().PrintLn("    pushq %rdx");
                }
            }
        }

        inferred = merged.value();
        return true;
    } else {
        ReportErrorForInfixExpression(ctx, gen_lhs.inferred(),
                                      gen_rhs.inferred(), expr.op().span());
        return false;
    }
}

static bool GenBooleanExpr(CodeGenContext &ctx,
                           std::shared_ptr<hir::Type> &inferred,
                           const hir::InfixExpression &expr) {
    auto &lhs = expr.lhs();
    auto &rhs = expr.rhs();

    ExprRValGen gen_lhs(ctx);
    lhs->Accept(gen_lhs);
    if (!gen_lhs) return false;

    ctx.lvar_table().SaveCalleeSize();

    ExprRValGen gen_rhs(ctx);
    rhs->Accept(gen_rhs);
    if (!gen_rhs) return false;

    if (gen_lhs.inferred()->IsBuiltin() && gen_rhs.inferred()->IsBuiltin()) {
        auto to = std::make_shared<hir::BuiltinType>(hir::BuiltinType::Bool,
                                                     expr.span());

        // Convert rhs to proper type, then pop it from stack
        if (!ImplicitlyConvertValueInStack(ctx, rhs->span(), gen_rhs.inferred(),
                                           to)) {
            return false;
        }
        ctx.lvar_table().SubCalleeSize(8);
        ctx.printer().PrintLn("    popq %rbx");

        // Free memory allocated by rhs.
        auto diff = ctx.lvar_table().RestoreCalleeSize();
        if (diff) ctx.printer().PrintLn("    addq ${}, %rsp", diff);

        // Convert lhs to proper type, then pop it from stack.
        if (!ImplicitlyConvertValueInStack(ctx, lhs->span(), gen_lhs.inferred(),
                                           to)) {
            return false;
        }

        if (expr.op().kind() == hir::InfixExpression::Op::Or) {
            ctx.printer().PrintLn("    orb %bl, (%rsp)");
        } else {
            ctx.printer().PrintLn("    andb %bl, (%rsp)");
        }

        inferred = to;
        return true;
    } else {
        ReportErrorForInfixExpression(ctx, gen_lhs.inferred(),
                                      gen_rhs.inferred(), expr.op().span());
        return false;
    }
}

static bool GenBitExpr(CodeGenContext &ctx,
                       std::shared_ptr<hir::Type> &inferred,
                       const hir::InfixExpression &expr) {
    auto &lhs = expr.lhs();
    auto &rhs = expr.rhs();

    ExprRValGen gen_lhs(ctx);
    lhs->Accept(gen_lhs);
    if (!gen_lhs) return false;

    ctx.lvar_table().SaveCalleeSize();

    ExprRValGen gen_rhs(ctx);
    rhs->Accept(gen_rhs);
    if (!gen_rhs) return false;

    if (gen_lhs.inferred()->IsBuiltin() && gen_rhs.inferred()->IsBuiltin()) {
        auto merged =
            ImplicitlyMergeTwoType(ctx, gen_lhs.inferred(), gen_rhs.inferred());
        if (!merged || !merged.value()->IsBuiltin() ||
            !merged.value()->ToBuiltin()->IsInteger()) {
            ReportErrorForInfixExpression(ctx, gen_lhs.inferred(),
                                          gen_rhs.inferred(), expr.op().span());
            return false;
        }

        auto builtin = merged.value()->ToBuiltin();
        TypeSizeCalc size(ctx);
        builtin->Accept(size);
        if (!size) return false;

        // Convert rhs to proper type, then pop it from stack
        if (!ImplicitlyConvertValueInStack(ctx, rhs->span(), gen_rhs.inferred(),
                                           merged.value())) {
            return false;
        }
        ctx.lvar_table().SubCalleeSize(8);
        ctx.printer().PrintLn("    popq %rbx");

        // Free memory allocated by rhs.
        auto diff = ctx.lvar_table().RestoreCalleeSize();
        if (diff) ctx.printer().PrintLn("    addq ${}, %rsp", diff);

        // Convert lhs to proper type.
        if (!ImplicitlyConvertValueInStack(ctx, lhs->span(), gen_lhs.inferred(),
                                           merged.value())) {
            return false;
        }

        if (expr.op().kind() == hir::InfixExpression::Op::BitAnd) {
            ctx.printer().PrintLn(
                "    {} {}, (%rsp)", AsmAnd(size.size()),
                Register(Register::BX).ToNameBySize(size.size()));
        } else if (expr.op().kind() == hir::InfixExpression::Op::BitOr) {
            ctx.printer().PrintLn(
                "    {} {}, (%rsp)", AsmOr(size.size()),
                Register(Register::BX).ToNameBySize(size.size()));
        } else {
            ctx.printer().PrintLn(
                "    {} {}, (%rsp)", AsmXor(size.size()),
                Register(Register::BX).ToNameBySize(size.size()));
        }

        inferred = merged.value();
        return true;
    } else {
        ReportErrorForInfixExpression(ctx, gen_lhs.inferred(),
                                      gen_rhs.inferred(), expr.op().span());
        return false;
    }
}

static bool GenComparsonExpr(CodeGenContext &ctx,
                             std::shared_ptr<hir::Type> &inferred,
                             const hir::InfixExpression &expr) {
    auto &lhs = expr.lhs();
    auto &rhs = expr.rhs();

    ExprRValGen gen_lhs(ctx);
    lhs->Accept(gen_lhs);
    if (!gen_lhs) return false;

    ctx.lvar_table().SaveCalleeSize();

    ExprRValGen gen_rhs(ctx);
    rhs->Accept(gen_rhs);
    if (!gen_rhs) return false;

    if ((gen_lhs.inferred()->IsBuiltin() && gen_rhs.inferred()->IsBuiltin()) ||
        (gen_lhs.inferred()->IsPointer() && gen_rhs.inferred()->IsPointer())) {
        auto merged =
            ImplicitlyMergeTwoType(ctx, gen_lhs.inferred(), gen_rhs.inferred());
        if (!merged ||
            (merged.value()->IsBuiltin() &&
             merged.value()->ToBuiltin()->kind() == hir::BuiltinType::Void)) {
            ReportErrorForInfixExpression(ctx, gen_lhs.inferred(),
                                          gen_rhs.inferred(), expr.op().span());
            return false;
        }

        // Check two types is merged correctly.
        assert(merged.value()->IsBuiltin() || merged.value()->IsPointer());

        // Relational operator cannot be used for non-integer type.
        if (merged.value()->IsPointer() ||
            !merged.value()->ToBuiltin()->IsInteger()) {
            if (expr.op().kind() != hir::InfixExpression::Op::EQ &&
                expr.op().kind() != hir::InfixExpression::Op::NE) {
                ReportErrorForInfixExpression(ctx, gen_lhs.inferred(),
                                              gen_rhs.inferred(),
                                              expr.op().span());
                return false;
            }
        }

        TypeSizeCalc size(ctx);
        merged.value()->Accept(size);
        if (!size) return false;

        // Convert rhs to proper type, then pop it from stack adn store to rbx.
        if (!ImplicitlyConvertValueInStack(ctx, rhs->span(), gen_rhs.inferred(),
                                           merged.value())) {
            return false;
        }
        ctx.lvar_table().SubCalleeSize(8);
        ctx.printer().PrintLn("    popq %rbx");

        // Free memory allocated by rhs.
        auto diff = ctx.lvar_table().RestoreCalleeSize();
        if (diff) ctx.printer().PrintLn("    addq ${}, %rsp", diff);

        // Convert lhs to proper type.
        if (!ImplicitlyConvertValueInStack(ctx, lhs->span(), gen_lhs.inferred(),
                                           merged.value())) {
            return false;
        }

        // Compare rax and rbx
        if (expr.op().kind() == hir::InfixExpression::Op::GT ||
            expr.op().kind() == hir::InfixExpression::Op::GE) {
            ctx.printer().PrintLn(
                "    {} (%rsp), {}", AsmCmp(size.size()),
                Register(Register::BX).ToNameBySize(size.size()));
        } else {
            ctx.printer().PrintLn(
                "    {} {}, (%rsp)", AsmCmp(size.size()),
                Register(Register::BX).ToNameBySize(size.size()));
        }

        if (expr.op().kind() == hir::InfixExpression::Op::EQ) {
            ctx.printer().PrintLn("    sete %al");
        } else if (expr.op().kind() == hir::InfixExpression::Op::NE) {
            ctx.printer().PrintLn("    setne %al");
        } else if (expr.op().kind() == hir::InfixExpression::Op::LT ||
                   expr.op().kind() == hir::InfixExpression::Op::GT) {
            ctx.printer().PrintLn("    setl %al");
        } else {
            ctx.printer().PrintLn("    setle %al");
        }
        ctx.printer().PrintLn("    movzbq %al, %rax");
        ctx.printer().PrintLn("    movq %rax, (%rsp)");

        inferred = std::make_shared<hir::BuiltinType>(hir::BuiltinType::Bool,
                                                      expr.span());
        return true;
    } else {
        ReportErrorForInfixExpression(ctx, gen_lhs.inferred(),
                                      gen_rhs.inferred(), expr.op().span());
        return false;
    }
}

static bool GenShiftExpr(CodeGenContext &ctx,
                         std::shared_ptr<hir::Type> &inferred,
                         const hir::InfixExpression &expr) {
    auto &lhs = expr.lhs();
    auto &rhs = expr.rhs();

    ExprRValGen gen_lhs(ctx);
    lhs->Accept(gen_lhs);
    if (!gen_lhs) return false;

    ctx.lvar_table().SaveCalleeSize();

    ExprRValGen gen_rhs(ctx);
    rhs->Accept(gen_rhs);
    if (!gen_rhs) return false;

    if (gen_lhs.inferred()->IsBuiltin() && gen_rhs.inferred()->IsBuiltin()) {
        auto merged =
            ImplicitlyMergeTwoType(ctx, gen_lhs.inferred(), gen_rhs.inferred());
        if (!merged || !merged.value()->IsBuiltin() ||
            !merged.value()->ToBuiltin()->IsInteger()) {
            ReportErrorForInfixExpression(ctx, gen_lhs.inferred(),
                                          gen_rhs.inferred(), expr.op().span());
            return false;
        }

        auto builtin = merged.value()->ToBuiltin();
        TypeSizeCalc size(ctx);
        builtin->Accept(size);
        if (!size) return false;

        // Convert rhs to proper type, then pop it from stack
        if (!ImplicitlyConvertValueInStack(ctx, rhs->span(), gen_rhs.inferred(),
                                           merged.value())) {
            return false;
        }
        ctx.lvar_table().SubCalleeSize(8);
        ctx.printer().PrintLn("    popq %rcx");

        // Free memory allocated by rhs.
        auto diff = ctx.lvar_table().RestoreCalleeSize();
        if (diff) ctx.printer().PrintLn("    addq ${}, %rsp", diff);

        // Convert lhs to proper type.
        if (!ImplicitlyConvertValueInStack(ctx, lhs->span(), gen_lhs.inferred(),
                                           merged.value())) {
            return false;
        }

        if (expr.op().kind() == hir::InfixExpression::Op::LShift) {
            ctx.printer().PrintLn("    {} %cl, (%rsp)",
                                  AsmLShift(builtin->IsSigned(), size.size()));
        } else {
            ctx.printer().PrintLn("    {} %cl, (%rsp)",
                                  AsmRShift(builtin->IsSigned(), size.size()));
        }

        inferred = merged.value();
        return true;
    } else {
        ReportErrorForInfixExpression(ctx, gen_lhs.inferred(),
                                      gen_rhs.inferred(), expr.op().span());
        return false;
    }
}

void ExprRValGen::Visit(const hir::InfixExpression &expr) {
    if (expr.op().kind() == hir::InfixExpression::Op::Assign) {
        success_ = GenAssignExpr(ctx_, inferred_, expr.lhs(), expr.rhs());
    } else if (expr.op().kind() == hir::InfixExpression::Op::Add ||
               expr.op().kind() == hir::InfixExpression::Op::Sub) {
        success_ = GenAdditiveExpr(ctx_, inferred_, expr);
    } else if (expr.op().kind() == hir::InfixExpression::Op::Mul ||
               expr.op().kind() == hir::InfixExpression::Op::Div ||
               expr.op().kind() == hir::InfixExpression::Op::Mod) {
        success_ = GenMultiplicativeExpr(ctx_, inferred_, expr);
    } else if (expr.op().kind() == hir::InfixExpression::Op::Or ||
               expr.op().kind() == hir::InfixExpression::Op::And) {
        success_ = GenBooleanExpr(ctx_, inferred_, expr);
    } else if (expr.op().kind() == hir::InfixExpression::Op::BitOr ||
               expr.op().kind() == hir::InfixExpression::Op::BitAnd ||
               expr.op().kind() == hir::InfixExpression::Op::BitXor) {
        success_ = GenBitExpr(ctx_, inferred_, expr);
    } else if (expr.op().kind() == hir::InfixExpression::Op::EQ ||
               expr.op().kind() == hir::InfixExpression::Op::NE ||
               expr.op().kind() == hir::InfixExpression::Op::LT ||
               expr.op().kind() == hir::InfixExpression::Op::LE ||
               expr.op().kind() == hir::InfixExpression::Op::GT ||
               expr.op().kind() == hir::InfixExpression::Op::GE) {
        success_ = GenComparsonExpr(ctx_, inferred_, expr);
    } else if (expr.op().kind() == hir::InfixExpression::Op::LShift ||
               expr.op().kind() == hir::InfixExpression::Op::RShift) {
        success_ = GenShiftExpr(ctx_, inferred_, expr);
    } else {
        FatalError("unreachable");
    }
}

void ExprRValGen::Visit(const hir::IndexExpression &expr) {
    ExprLValGen gen_addr(ctx_);
    expr.Accept(gen_addr);
    if (!gen_addr) return;

    // No operation required if the field is fat object, as it required to store
    // its address to stack, and is already done.
    if (!IsFatObject(ctx_, gen_addr.inferred())) {
        // We expect non-fat object can be stored to register.
        TypeSizeCalc of_size(ctx_);
        gen_addr.inferred()->Accept(of_size);
        if (!of_size) return;
        assert(of_size.size() <= 8);

        // Just copy value that the calculated address pointing.
        ctx_.printer().PrintLn("    popq %rax");
        ctx_.printer().PrintLn("    pushq (%rax)");
    }

    inferred_ = gen_addr.inferred();
    success_ = true;
}

void ExprRValGen::Visit(const hir::CallExpression &expr) {
    auto var = IsVariable(expr.func());
    if (var && ctx_.func_info_table().Exists(var.value())) {
        auto &callee_info = ctx_.func_info_table().Query(var.value());

        if (!callee_info.has_variadic() &&
            callee_info.params().size() != expr.args().size()) {
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

        // Assign register or stack for each argument.
        ArgumentAssignmentTable arg_table;
        if (!arg_table.Build(ctx_, callee_table.Exists(callee_table.ret_name),
                             expr.args(), callee_info.params(),
                             callee_info.has_variadic())) {
            return;
        }

        // Allocate stack for arguments.
        AllocateAlignedStackMemory(ctx_, arg_table.StackSize(), 16);

        // Offset to the arguments block.
        const auto offset = caller_table.CalleeSize();

        // Prepare arguments.
        for (const auto &entry : arg_table.Entries()) {
            caller_table.SaveCalleeSize();
            ExprRValGen gen(ctx_, entry.array_base_type());
            entry.arg()->Accept(gen);
            if (!gen) return;

            // Convert generated value to expected type.
            if (!ImplicitlyConvertValueInStack(ctx_, entry.arg()->span(),
                                               gen.inferred_,
                                               entry.expect_type())) {
                return;
            }

            if (entry.kind() == ArgumentAssignmentTable::Entry::Reg) {
                ctx_.lvar_table().SubCalleeSize(8);
                ctx_.printer().PrintLn("    popq {}",
                                       entry.reg()->ToQuadName());
            } else {
                TypeSizeCalc size(ctx_);
                entry.expect_type()->Accept(size);
                if (!size) return;

                // NOTE:
                // As param_info.Offset is relative position from the addres of
                // arguments block.
                // But currently `gen` allocated memory and rsp doesn't point to
                // the block, so I need to add `alloc_size` to
                // `param_info.Offset` to get proper address.
                auto src_offset = -ctx_.lvar_table().CalleeSize();
                auto dst_offset = -offset + entry.offset();
                IndexableAsmRegPtr src(Register::BP, src_offset);
                IndexableAsmRegPtr dst(Register::BP, dst_offset);
                CopyBytes(ctx_, src, dst, RoundUp(size.size(), 8));
            }

            // Free allocated memory.
            auto diff = caller_table.RestoreCalleeSize();
            if (diff) ctx_.printer().PrintLn("    addq ${}, %rsp", diff);
        }

        // If return value needs caller-allocated memory, move the address to
        // rdi.
        if (callee_table.Exists(callee_table.ret_name)) {
            auto &entry = callee_table.Query(callee_table.ret_name);
            ctx_.printer().PrintLn("    leaq {}(%rbp), %rdi",
                                   -offset + entry.Offset());
        }

        // This compiler doesn't use floating point number at a time.
        // So, set %al to 0 as system v abi claim it.
        ctx_.printer().PrintLn("    movb $0, %al");

        if (callee_info.is_outer())
            ctx_.printer().PrintLn("    callq {}@PLT", var.value());
        else
            ctx_.printer().PrintLn("    callq {}", var.value());

        // Ensure push returned value.
        caller_table.AddCalleeSize(8);
        ctx_.printer().PrintLn("    pushq %rax");

        inferred_ = ctx_.func_info_table().Query(var.value()).ret_type();
        success_ = true;
    } else {
        ReportInfo info(expr.func()->span(), "not a callable", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
    }
}

void ExprRValGen::Visit(const hir::AccessExpression &expr) {
    ExprLValGen gen_addr(ctx_);
    expr.Accept(gen_addr);
    if (!gen_addr) return;

    // No operation required if the element is fat object, as it required to
    // store its address to stack, and is already done.
    if (!IsFatObject(ctx_, gen_addr.inferred())) {
        // We expect non-fat object can be stored to register.
        TypeSizeCalc field_size(ctx_);
        gen_addr.inferred()->Accept(field_size);
        if (!field_size) return;
        assert(field_size.size() <= 8);

        // Just copy value that the calculated address pointing.
        ctx_.printer().PrintLn("    popq %rax");
        ctx_.printer().PrintLn("    pushq (%rax)");
    }

    inferred_ = gen_addr.inferred();
    success_ = true;
}

void ExprRValGen::Visit(const hir::CastExpression &expr) {
    ExprRValGen gen(ctx_);
    expr.expr()->Accept(gen);
    if (!gen) return;

    // No conversion in assembly code.
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
    ctx_.SuppressOutput();

    ExprRValGen gen(ctx_);
    expr.expr()->Accept(gen);
    if (!gen) return;

    TypeSizeCalc size(ctx_);
    gen.inferred_->Accept(size);
    if (!size) return;

    ctx_.ActivateOutput();

    ctx_.printer().PrintLn("    pushq ${}", size.size());

    inferred_ = std::make_shared<hir::BuiltinType>(hir::BuiltinType::USize,
                                                   expr.span());
    success_ = true;
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

    if (IsFatObject(ctx_, entry.type())) {
        // If the variable holds fat object, just returns its address.
        ExprLValGen gen_addr(ctx_);
        expr.Accept(gen_addr);
        if (!gen_addr) return;
    } else {
        // non-fat object can be stored to register.
        TypeSizeCalc size(ctx_);
        entry.type()->Accept(size);
        if (!size) return;
        assert(size.size() <= 8);

        // Push value the variable holds.
        ctx_.lvar_table().AddCalleeSize(8);
        ctx_.printer().PrintLn("    pushq {}", entry.AsmRepr().ToAsmRepr(0, 8));
    }

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

void ExprRValGen::Visit(const hir::NullPtrExpression &expr) {
    ctx_.lvar_table().AddCalleeSize(8);
    ctx_.printer().PrintLn("    pushq ${}", 0);

    auto of =
        std::make_shared<hir::BuiltinType>(hir::BuiltinType::Void, expr.span());
    inferred_ = std::make_shared<hir::PointerType>(of, expr.span());
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
        init.value()->Accept(gen);
        if (!gen) return;

        // Convert generated value in stack so can be used to initialize field.
        if (!ImplicitlyConvertValueInStack(ctx_, init.span(), gen.inferred_,
                                           field.type())) {
            return;
        }

        TypeSizeCalc field_size(ctx_);
        field.type()->Accept(field_size);
        if (!field_size) return;

        IndexableAsmRegPtr src(Register::BP, -ctx_.lvar_table().CalleeSize());
        IndexableAsmRegPtr dst(Register::BP, -offset + field.Offset());
        CopyBytes(ctx_, src, dst, field_size.size());

        // Free temporary generate value.
        auto diff = ctx_.lvar_table().RestoreCalleeSize();
        if (diff) ctx_.printer().PrintLn("    addq ${}, %rsp", diff);
    }

    // As array is manipulated by using its pointer, push pointer to array
    // object.
    ctx_.lvar_table().AddCalleeSize(8);
    ctx_.printer().PrintLn("    pushq %rsp");

    inferred_ = std::make_shared<hir::NameType>(type);
    success_ = true;
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
        std::optional<std::shared_ptr<hir::Type>> of;
        if (array_base_type_.value()->IsArray()) {
            of = array_base_type_.value()->ToArray()->of();
        }

        ctx_.lvar_table().SaveCalleeSize();
        ExprRValGen gen(ctx_, of);
        expr.inits().at(i)->Accept(gen);
        if (!gen) return;

        if (!ImplicitlyConvertValueInStack(ctx_, expr.inits().at(i)->span(),
                                           gen.inferred_,
                                           array_base_type_.value())) {
            return;
        }

        if (IsFatObject(ctx_, array_base_type_.value())) {
            ctx_.printer().PrintLn("    movq -{}(%rbp), %rax",
                                   ctx_.lvar_table().CalleeSize());

            // Copy generated value.
            IndexableAsmRegPtr src(Register::AX, 0);
            IndexableAsmRegPtr dst(Register::BP,
                                   -offset + i * base_size.size());
            CopyBytes(ctx_, src, dst, base_size.size());
        } else {
            assert(base_size.size() <= 8);
            int64_t elem_offset = -offset + i * base_size.size();
            ctx_.lvar_table().SubCalleeSize(8);
            ctx_.printer().PrintLn("    popq %rax");
            ctx_.printer().PrintLn("    movq %rax, {}(%rbp)", elem_offset);
        }

        // Free temporary generate value.
        auto diff = ctx_.lvar_table().RestoreCalleeSize();
        if (diff) ctx_.printer().PrintLn("    addq ${}, %rsp", diff);
    }

    // As struct is manipulated by using its pointer, push pointer to struct
    // object.
    ctx_.lvar_table().AddCalleeSize(8);
    ctx_.printer().PrintLn("    pushq %rsp");

    inferred_ = std::make_shared<hir::ArrayType>(
        array_base_type_.value(), expr.inits().size(), expr.span());
    success_ = true;
}

void ExprLValGen::Visit(const hir::UnaryExpression &expr) {
    if (expr.op().kind() == hir::UnaryExpression::Op::Deref) {
        ExprRValGen gen_addr(ctx_);
        expr.expr()->Accept(gen_addr);
        if (!gen_addr) return;

        if (!gen_addr.inferred()->IsPointer()) {
            ReportInfo info(expr.span(), "cannot deref non-pointer", "");
            Report(ctx_.ctx(), ReportLevel::Error, info);
            return;
        }

        inferred_ = gen_addr.inferred()->ToPointer()->of();
        success_ = true;
    } else {
        ReportInfo info(expr.span(), "invalid unary operator for lvalue", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
    }
}

void ExprLValGen::Visit(const hir::InfixExpression &expr) {
    ReportInfo info(expr.span(), "not a lvalue", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprLValGen::Visit(const hir::IndexExpression &expr) {
    ExprRValGen gen_addr(ctx_);
    expr.expr()->Accept(gen_addr);
    if (!gen_addr) return;

    if (!gen_addr.inferred()->IsArray() && !gen_addr.inferred()->IsPointer()) {
        ReportInfo info(expr.expr()->span(), "invalid indexing",
                        "not a array or pointer");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    // Here, top of stack is pointer to array, because
    // - If it is pointer to array, it trivial.
    // - If it is fat object, it also pushes pointer to it.

    std::shared_ptr<hir::Type> of;
    if (gen_addr.inferred()->IsArray()) {
        of = gen_addr.inferred()->ToArray()->of();
    } else {
        of = gen_addr.inferred()->ToPointer()->of();
    }

    TypeSizeCalc of_size(ctx_);
    of->Accept(of_size);
    if (!of_size) return;

    ctx_.lvar_table().SaveCalleeSize();

    ExprRValGen gen_index(ctx_, std::nullopt);
    expr.index()->Accept(gen_index);
    if (!gen_index) return;

    // Convert index to usize.
    auto to = std::make_shared<hir::BuiltinType>(hir::BuiltinType::USize,
                                                 expr.index()->span());
    if (!ImplicitlyConvertValueInStack(ctx_, expr.index()->span(),
                                       gen_index.inferred(), to)) {
        return;
    }

    // Pop index.
    ctx_.lvar_table().SubCalleeSize(8);
    ctx_.printer().PrintLn("    popq %rax");

    // Calculate offset to the element
    ctx_.printer().PrintLn("    movq ${}, %rbx", of_size.size());
    ctx_.printer().PrintLn("    mulq %rbx");

    // Free memory allocated by rhs so top of stack to be generated address.
    auto diff = ctx_.lvar_table().RestoreCalleeSize();
    if (diff) ctx_.printer().PrintLn("    addq ${}, %rsp", diff);

    // Change address to point to element.
    ctx_.printer().PrintLn("    addq %rax, (%rsp)");

    inferred_ = of;
    success_ = true;
}

void ExprLValGen::Visit(const hir::CallExpression &expr) {
    ReportInfo info(expr.span(), "doesn't have address", "");
    Report(ctx_.ctx(), ReportLevel::Error, info);
}

void ExprLValGen::Visit(const hir::AccessExpression &expr) {
    ExprRValGen gen_addr(ctx_);
    expr.expr()->Accept(gen_addr);
    if (!gen_addr) return;

    auto type = gen_addr.inferred();
    if (type->IsPointer()) {
        type = type->ToPointer()->of();
    }

    if (!type->IsName()) {
        auto spec =
            fmt::format("{} is not a struct", gen_addr.inferred()->ToString());
        ReportInfo info(expr.expr()->span(), "invalid struct access",
                        std::move(spec));
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }
    auto &name = type->ToName()->value();

    // Here, top of stack is pointer to struct, because
    // - If it is pointer to struct, it trivial.
    // - If it is fat object, it also pushes pointer to it.

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

    inferred_ = field.type();
    success_ = true;
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
    ctx_.printer().PrintLn("    leaq {}, %rax",
                           entry.AsmRepr().ToAsmRepr(0, 8));
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

void ExprLValGen::Visit(const hir::NullPtrExpression &expr) {
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

bool ImplicitlyConvertValueInStack(CodeGenContext &ctx, Span value_span,
                                   const std::shared_ptr<hir::Type> &from,
                                   const std::shared_ptr<hir::Type> &to) {
    if (from->IsBuiltin()) {
        if (to->IsBuiltin()) {
            bool conversion_happen = false;
            auto from_kind = from->ToBuiltin()->kind();
            auto to_kind = to->ToBuiltin()->kind();
            if (from_kind == hir::BuiltinType::UInt8) {
                if (to_kind == hir::BuiltinType::UInt8) {
                    // no conversion
                } else if (to_kind == hir::BuiltinType::UInt16) {
                    ctx.printer().PrintLn("    movzbw (%rsp), %ax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::UInt32) {
                    ctx.printer().PrintLn("    movzbl (%rsp), %eax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::UInt64) {
                    ctx.printer().PrintLn("    movzbq (%rsp), %rax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::USize) {
                    ctx.printer().PrintLn("    movzbq (%rsp), %rax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int16) {
                    ctx.printer().PrintLn("    movsbw (%rsp), %ax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int32) {
                    ctx.printer().PrintLn("    movsbl (%rsp), %eax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int64) {
                    ctx.printer().PrintLn("    movsbq (%rsp), %rax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::ISize) {
                    ctx.printer().PrintLn("    movsbq (%rsp), %rax");
                    conversion_happen = true;
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::UInt16) {
                if (to_kind == hir::BuiltinType::UInt16) {
                    // no conversion
                } else if (to_kind == hir::BuiltinType::UInt32) {
                    ctx.printer().PrintLn("    movzwl (%rsp), %eax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::UInt64) {
                    ctx.printer().PrintLn("    movzwq (%rsp), %rax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::USize) {
                    ctx.printer().PrintLn("    movzwq (%rsp), %rax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int32) {
                    ctx.printer().PrintLn("    movswl (%rsp), %eax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int64) {
                    ctx.printer().PrintLn("    movswq (%rsp), %rax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::ISize) {
                    ctx.printer().PrintLn("    movswq (%rsp), %rax");
                    conversion_happen = true;
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::UInt32) {
                if (to_kind == hir::BuiltinType::UInt32) {
                    // no conversion
                } else if (to_kind == hir::BuiltinType::UInt64) {
                    // no conversion, as movzlq doesn't exists
                } else if (to_kind == hir::BuiltinType::USize) {
                    // no conversion, as movzlq doesn't exists
                } else if (to_kind == hir::BuiltinType::Int64) {
                    ctx.printer().PrintLn("    movslq (%rsp), %rax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::ISize) {
                    ctx.printer().PrintLn("    movslq (%rsp), %rax");
                    conversion_happen = true;
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::UInt64) {
                if (to_kind == hir::BuiltinType::UInt64) {
                    // no conversion
                } else if (to_kind == hir::BuiltinType::USize) {
                    // no conversion
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::USize) {
                if (to_kind == hir::BuiltinType::UInt64) {
                    // no conversion
                } else if (to_kind == hir::BuiltinType::USize) {
                    // no conversion
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::Int8) {
                if (to_kind == hir::BuiltinType::Int8) {
                    // no conversion
                } else if (to_kind == hir::BuiltinType::Int16) {
                    ctx.printer().PrintLn("    movsbw (%rsp), %ax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int32) {
                    ctx.printer().PrintLn("    movsbl (%rsp), %eax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int64) {
                    ctx.printer().PrintLn("    movsbq (%rsp), %rax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::ISize) {
                    ctx.printer().PrintLn("    movsbq (%rsp), %rax");
                    conversion_happen = true;
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::Int16) {
                if (to_kind == hir::BuiltinType::Int16) {
                    // no conversion
                } else if (to_kind == hir::BuiltinType::Int32) {
                    ctx.printer().PrintLn("    movswl (%rsp), %eax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::Int64) {
                    ctx.printer().PrintLn("    movswq (%rsp), %rax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::ISize) {
                    ctx.printer().PrintLn("    movswq (%rsp), %rax");
                    conversion_happen = true;
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::Int32) {
                if (to_kind == hir::BuiltinType::Int32) {
                    // no conversion
                } else if (to_kind == hir::BuiltinType::Int64) {
                    ctx.printer().PrintLn("    movslq (%rsp), %rax");
                    conversion_happen = true;
                } else if (to_kind == hir::BuiltinType::ISize) {
                    ctx.printer().PrintLn("    movslq (%rsp), %rax");
                    conversion_happen = true;
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::Int64) {
                if (to_kind == hir::BuiltinType::Int64) {
                    // no conversion
                } else if (to_kind == hir::BuiltinType::ISize) {
                    // no conversion
                } else {
                    goto failed;
                }
            } else if (from_kind == hir::BuiltinType::ISize) {
                if (to_kind == hir::BuiltinType::Int64) {
                    // no conversion
                } else if (to_kind == hir::BuiltinType::ISize) {
                    // no conversion
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
            if (conversion_happen)
                ctx.printer().PrintLn("    movq %rax, (%rsp)");
            return true;
        } else {
            goto failed;
        }
    } else if (from->IsPointer()) {
        if (to->ToPointer()) {
            auto from_of = from->ToPointer()->of();
            auto to_of = to->ToPointer()->of();
            if (from_of->IsBuiltin() &&
                from_of->ToBuiltin()->kind() == hir::BuiltinType::Void) {
                return true;
            } else if (*from_of == *to_of) {
                return true;
            } else {
                goto failed;
            }
        } else {
            goto failed;
        }
    } else if (from->IsArray()) {
        if (to->IsArray()) {
            if (*from == *to) {
                return true;
            } else {
                goto failed;
            }
        } else if (to->IsPointer()) {
            auto from_of = from->ToArray()->of();
            auto to_of = to->ToPointer()->of();
            if (*from_of == *to_of) {
                return true;
            } else {
                goto failed;
            }
        } else {
            goto failed;
        }
    } else if (from->IsName()) {
        if (*from == *to) {
            return true;
        } else {
            goto failed;
        }
    } else {
        FatalError("unreachable");
    }

failed:
    auto spec = fmt::format("cannot convert this {} to {} implicitly",
                            from->ToString(), to->ToString());
    ReportInfo info(value_span, "implicit conversion failed", std::move(spec));
    Report(ctx.ctx(), ReportLevel::Error, info);
    return false;
}

}  // namespace mini
