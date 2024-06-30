#include "type.h"

#include <cstdint>
#include <memory>
#include <stdexcept>

#include "../report.h"
#include "eval.h"

void SymbolTableEntrySizeCalculator::visit(const VariableEntry &entry) {
    TypeSizeCalculator calc(ctx_, os_);
    entry.type()->accept(calc);
    size_ = calc.size();
}

void SymbolTableEntrySizeCalculator::visit(const StructEntry &entry) {
    uint64_t alignment = 0;
    for (const auto &field : entry.fields()) {
        TypeSizeCalculator calc(ctx_, os_);
        field.type()->accept(calc);
        if (!(success_ &= calc.success())) return;
        if (alignment < calc.size()) alignment = calc.size();
    }
    size_ = alignment * entry.fields().size();
}

void TypeSizeCalculator::visit(const ArrayType &type) {
    TypeSizeCalculator base_calc(ctx_, os_);
    type.of()->accept(base_calc);
    if (!(success_ &= base_calc.success_)) return;
    uint64_t base_size = base_calc.size_;

    ExprConstEval len_eval(ctx_, os_);
    type.size()->accept(len_eval);
    if (!(success_ &= len_eval.success())) return;
    uint64_t len = len_eval.value();

    size_ = base_size * len;
}

void TypeSizeCalculator::visit(const NameType &type) {
    try {
        auto &entry = ctx_.symbol_table()->query(type.name());
        SymbolTableEntrySizeCalculator entry_size(ctx_, os_);
        entry->accept(entry_size);
        if (!(success_ &= entry_size.success())) return;
        size_ = entry_size.size();
    } catch (std::out_of_range &e) {
        ReportInfo info(type.span(), "no such variable exists", "");
        report(ctx_, ReportLevel::Error, info);
    }
}

void ExprTypeInferencer::visit(const UnaryExpression &expr) {}

void ExprTypeInferencer::visit(const InfixExpression &expr) {}

void ExprTypeInferencer::visit(const IndexExpression &expr) {}

void ExprTypeInferencer::visit(const CallExpression &expr) {}

void ExprTypeInferencer::visit(const AccessExpression &expr) {
    ExprTypeInferencer body_infere(ctx_, os_);
    expr.expr_->accept(body_infere);
    if (!(success_ &= body_infere.success_)) return;
}

void ExprTypeInferencer::visit(const CastExpression &expr) {
    success_ = true;
    inferred_ = expr.type();
}

void ExprTypeInferencer::visit(const ESizeofExpression &expr) {
    success_ = true;
    inferred_ = std::make_shared<UIntType>(expr.span());
}

void ExprTypeInferencer::visit(const TSizeofExpression &expr) {
    success_ = true;
    inferred_ = std::make_shared<UIntType>(expr.span());
}

void ExprTypeInferencer::visit(const EnumSelectExpression &expr) {
    success_ = true;
    inferred_ =
        std::make_shared<NameType>(std::string(expr.src_.name()), expr.span());
}

void ExprTypeInferencer::visit(const VariableExpression &expr) {
    class VariableTypeGetter : public SymbolTableEntryVisitor {
    public:
        VariableTypeGetter(Context &ctx, std::ostream &os)
            : success_(false), type_(nullptr), ctx_(ctx), os_(os) {}
        bool success() const { return success_; }
        std::shared_ptr<Type> type() const { return type_; }
        void visit(const VariableEntry &entry) override {
            success_ = true;
            type_ = entry.type();
        }
        void visit(const StructEntry &entry) override {
            ReportInfo info(entry.span(), "not a variable", "");
            report(ctx_, ReportLevel::Error, info);
        }
        void visit(const EnumEntry &entry) override {
            ReportInfo info(entry.span(), "not a variable", "");
            report(ctx_, ReportLevel::Error, info);
        }
        void visit(const FunctionEntry &entry) override {
            ReportInfo info(entry.span(), "not a variable", "");
            report(ctx_, ReportLevel::Error, info);
        }

    private:
        bool success_;
        std::shared_ptr<Type> type_;
        Context &ctx_;
        std::ostream &os_;
    };

    try {
        auto &entry = ctx_.symbol_table()->query(expr.value());
        VariableTypeGetter getter(ctx_, os_);
        entry->accept(getter);
        if (!(success_ &= getter.success())) return;
        inferred_ = getter.type();
    } catch (std::out_of_range &e) {
        ReportInfo info(expr.span(), "no such variable exists", "");
        report(ctx_, ReportLevel::Error, info);
    }
}

void ExprTypeInferencer::visit(const IntegerExpression &expr) {
    success_ = true;
    inferred_ = std::make_shared<UIntLiteralType>(expr.span());
}

void ExprTypeInferencer::visit(const StringExpression &expr) {
    auto span = expr.span();
    auto size = std::make_shared<IntegerExpression>(expr.value().size(), span);
    auto of = std::make_shared<CharType>(span);
    success_ = true;
    inferred_ = std::make_shared<ArrayType>(LParen(span), of, RParen(span),
                                            LSquare(span), size, RSquare(span));
}

void ExprTypeInferencer::visit(const BoolExpression &expr) {
    success_ = true;
    inferred_ = std::make_shared<BoolType>(expr.span());
}
