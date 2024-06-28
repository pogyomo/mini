#include "type.h"

#include <cstdint>

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
