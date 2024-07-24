#include "type.h"

#include <algorithm>

#include "../report.h"

namespace mini {

void TypeAlignCalc::Visit(const hir::BuiltinType &type) {
    if (type.kind() == hir::BuiltinType::Void) {
        align_ = 0;
    } else if (type.kind() == hir::BuiltinType::ISize) {
        align_ = 8;
    } else if (type.kind() == hir::BuiltinType::Int8) {
        align_ = 1;
    } else if (type.kind() == hir::BuiltinType::Int16) {
        align_ = 2;
    } else if (type.kind() == hir::BuiltinType::Int32) {
        align_ = 4;
    } else if (type.kind() == hir::BuiltinType::Int64) {
        align_ = 8;
    } else if (type.kind() == hir::BuiltinType::USize) {
        align_ = 8;
    } else if (type.kind() == hir::BuiltinType::UInt8) {
        align_ = 1;
    } else if (type.kind() == hir::BuiltinType::UInt16) {
        align_ = 2;
    } else if (type.kind() == hir::BuiltinType::UInt32) {
        align_ = 4;
    } else if (type.kind() == hir::BuiltinType::UInt64) {
        align_ = 8;
    } else if (type.kind() == hir::BuiltinType::Char) {
        align_ = 1;
    } else if (type.kind() == hir::BuiltinType::Bool) {
        align_ = 1;
    } else {
        FatalError("unreachable");
    }
    success_ = true;
}

void TypeAlignCalc::Visit(const hir::PointerType &) {
    align_ = 8;
    success_ = true;
}

void TypeAlignCalc::Visit(const hir::ArrayType &type) {
    TypeAlignCalc calc(ctx_);
    type.of()->Accept(calc);
    if (!calc) return;

    align_ = calc.align_;
    success_ = true;
}

void TypeAlignCalc::Visit(const hir::NameType &type) {
    if (ctx_.struct_table().Exists(type.value())) {
        auto &entry = ctx_.struct_table().Query(type.value());
        if (entry.AlignCalculated()) {
            align_ = entry.align();
            success_ = true;
            return;
        }

        align_ = 0;
        for (const auto &[_, field] : entry) {
            TypeAlignCalc calc(ctx_);
            field.type()->Accept(calc);
            if (!calc) return;

            align_ = std::max(align_, calc.align_);
        }
        success_ = true;
    } else if (ctx_.enum_table().Exists(type.value())) {
        align_ = 8;
        success_ = true;
    } else {
        ReportInfo info(type.span(), "no such type exists", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
    }
}

void TypeSizeCalc::Visit(const hir::BuiltinType &type) {
    if (type.kind() == hir::BuiltinType::Void) {
        size_ = 0;
    } else if (type.kind() == hir::BuiltinType::ISize) {
        size_ = 8;
    } else if (type.kind() == hir::BuiltinType::Int8) {
        size_ = 1;
    } else if (type.kind() == hir::BuiltinType::Int16) {
        size_ = 2;
    } else if (type.kind() == hir::BuiltinType::Int32) {
        size_ = 4;
    } else if (type.kind() == hir::BuiltinType::Int64) {
        size_ = 8;
    } else if (type.kind() == hir::BuiltinType::USize) {
        size_ = 8;
    } else if (type.kind() == hir::BuiltinType::UInt8) {
        size_ = 1;
    } else if (type.kind() == hir::BuiltinType::UInt16) {
        size_ = 2;
    } else if (type.kind() == hir::BuiltinType::UInt32) {
        size_ = 4;
    } else if (type.kind() == hir::BuiltinType::UInt64) {
        size_ = 8;
    } else if (type.kind() == hir::BuiltinType::Char) {
        size_ = 1;
    } else if (type.kind() == hir::BuiltinType::Bool) {
        size_ = 1;
    } else {
        FatalError("unreachable");
    }
    success_ = true;
}

void TypeSizeCalc::Visit(const hir::PointerType &) {
    size_ = 8;
    success_ = true;
}

void TypeSizeCalc::Visit(const hir::ArrayType &type) {
    if (!type.size()) {
        ReportInfo info(type.span(), "unsized array", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
        return;
    }

    TypeSizeCalc calc(ctx_);
    type.of()->Accept(calc);
    if (!calc) return;

    size_ = calc.size_ * type.size().value();
    success_ = true;
}

void TypeSizeCalc::Visit(const hir::NameType &type) {
    if (ctx_.struct_table().Exists(type.value())) {
        auto &entry = ctx_.struct_table().Query(type.value());
        if (entry.SizeAndOffsetCalculated()) {
            size_ = entry.size();
            success_ = true;
            return;
        }

        size_ = 0;
        uint64_t align = 0;
        for (auto &[name, field] : entry) {
            TypeAlignCalc align_calc(ctx_);
            field.type()->Accept(align_calc);
            if (!align_calc) return;

            TypeSizeCalc size_calc(ctx_);
            field.type()->Accept(size_calc);
            if (!size_calc) return;

            while (size_ % align_calc.align() != 0) size_++;

            field.set_offset(size_);
            size_ += size_calc.size_;
            align = std::max(align, align_calc.align());
        }
        while (size_ % align != 0) size_++;

        if (!entry.AlignCalculated()) {
            entry.set_align(align);
            entry.MarkAsAlignCalculated();
        }
        entry.set_size(size_);
        entry.MarkAsSizeAndOffsetCalculated();

        success_ = true;
    } else if (ctx_.enum_table().Exists(type.value())) {
        size_ = 8;
        success_ = true;
    } else {
        ReportInfo info(type.span(), "no such type exists", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
    }
}

}  // namespace mini
