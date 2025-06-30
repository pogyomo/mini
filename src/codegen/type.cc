#include "type.h"

#include <algorithm>
#include <memory>
#include <string>

#include "../report.h"
#include "../span.h"

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
            align_ = entry.Align();
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
        TypeAlignCalc calc(ctx_);
        ctx_.enum_table().Query(type.value()).base_type()->Accept(calc);
        if (!calc) return;
        align_ = calc.align_;
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
            size_ = entry.Size();
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

            field.SetOffset(size_);
            size_ += size_calc.size_;
            align = std::max(align, align_calc.align());
        }
        while (size_ % align != 0) size_++;

        if (!entry.AlignCalculated()) {
            entry.SetAlign(align);
            entry.MarkAsAlignCalculated();
        }
        entry.SetSize(size_);
        entry.MarkAsSizeAndOffsetCalculated();

        success_ = true;
    } else if (ctx_.enum_table().Exists(type.value())) {
        TypeSizeCalc calc(ctx_);
        ctx_.enum_table().Query(type.value()).base_type()->Accept(calc);
        if (!calc) return;
        size_ = calc.size_;
        success_ = true;
    } else {
        ReportInfo info(type.span(), "no such type exists", "");
        Report(ctx_.ctx(), ReportLevel::Error, info);
    }
}

bool CalculateStructSizeAndOffset(CodeGenContext &ctx, const std::string &name,
                                  Span span) {
    // TypeSizeCalc internally cache the struct size and offset, so use it.
    TypeSizeCalc calc(ctx);
    hir::NameType(std::string(name), span).Accept(calc);
    return (bool)calc;
}

std::optional<std::shared_ptr<hir::Type>> ImplicitlyMergeTwoType(
    CodeGenContext &ctx, const std::shared_ptr<hir::Type> &t1,
    const std::shared_ptr<hir::Type> &t2) {
    if (t1->IsPointer()) {
        if (!t2->IsPointer()) goto failed;

        auto t1_of = t1->ToPointer()->of();
        auto t2_of = t2->ToPointer()->of();
        if (t1_of->IsBuiltin() &&
            t1_of->ToBuiltin()->kind() == hir::BuiltinType::Void) {
            return std::make_shared<hir::PointerType>(t2_of,
                                                      t1->span() + t2->span());
        } else if (t2_of->IsBuiltin() &&
                   t2_of->ToBuiltin()->kind() == hir::BuiltinType::Void) {
            return std::make_shared<hir::PointerType>(t1_of,
                                                      t1->span() + t2->span());
        } else if (*t1 == *t2) {
            return std::make_shared<hir::PointerType>(t1_of,
                                                      t1->span() + t2->span());
        } else {
            goto failed;
        }
    } else if (t1->IsName()) {
        if (!t2->IsName()) goto failed;

        if (t1->ToName()->value() == t2->ToName()->value()) {
            return std::make_shared<hir::NameType>(
                std::string(t1->ToName()->value()), t1->span() + t2->span());
        } else {
            goto failed;
        }
    } else if (t1->IsArray()) {
        auto t1_of = t1->ToArray()->of();
        if (t2->IsArray()) {
            if (*t1 == *t2) {
                return std::make_shared<hir::ArrayType>(
                    t1_of, t1->ToArray()->size(), t1->span() + t2->span());
            } else {
                goto failed;
            }
        } else if (t2->IsPointer()) {
            auto t2_of = t2->ToPointer()->of();
            if (*t1_of == *t2_of) {
                return std::make_shared<hir::PointerType>(
                    t2_of, t1->span() + t2->span());
            } else {
                return std::nullopt;
            }
        } else {
            goto failed;
        }
    } else {
        if (!t2->IsBuiltin()) goto failed;

        auto t1_kind = t1->ToBuiltin()->kind();
        auto t2_kind = t2->ToBuiltin()->kind();
        auto span = t1->span() + t2->span();
        if (t1_kind == hir::BuiltinType::UInt8) {
            if (t2_kind == hir::BuiltinType::UInt8) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt16) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt32) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt64) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::USize) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int8) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int16) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int32) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int64) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::ISize) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            }
        } else if (t1_kind == hir::BuiltinType::UInt16) {
            if (t2_kind == hir::BuiltinType::UInt8) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt16) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt32) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt64) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::USize) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int8) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::Int16, span);
            } else if (t2_kind == hir::BuiltinType::Int16) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int32) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int64) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::ISize) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            }
        } else if (t1_kind == hir::BuiltinType::UInt32) {
            if (t2_kind == hir::BuiltinType::UInt8) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt16) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt32) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt64) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::USize) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int8) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::Int32, span);
            } else if (t2_kind == hir::BuiltinType::Int16) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::Int32, span);
            } else if (t2_kind == hir::BuiltinType::Int32) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int64) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::ISize) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            }
        } else if (t1_kind == hir::BuiltinType::UInt64) {
            if (t2_kind == hir::BuiltinType::UInt8) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt16) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt32) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt64) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::USize) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int8) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::Int64, span);
            } else if (t2_kind == hir::BuiltinType::Int16) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::Int64, span);
            } else if (t2_kind == hir::BuiltinType::Int32) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::Int64, span);
            } else if (t2_kind == hir::BuiltinType::Int64) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::ISize) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            }
        } else if (t1_kind == hir::BuiltinType::USize) {
            if (t2_kind == hir::BuiltinType::UInt8) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt16) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt32) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt64) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::USize) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int8) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::Int64, span);
            } else if (t2_kind == hir::BuiltinType::Int16) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::Int64, span);
            } else if (t2_kind == hir::BuiltinType::Int32) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::Int64, span);
            } else if (t2_kind == hir::BuiltinType::Int64) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::ISize) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            }
        } else if (t1_kind == hir::BuiltinType::Int8) {
            if (t2_kind == hir::BuiltinType::UInt8) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt16) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::Int16, span);
            } else if (t2_kind == hir::BuiltinType::UInt32) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::Int32, span);
            } else if (t2_kind == hir::BuiltinType::UInt64) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::Int64, span);
            } else if (t2_kind == hir::BuiltinType::USize) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::ISize, span);
            } else if (t2_kind == hir::BuiltinType::Int8) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int16) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int32) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int64) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::ISize) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            }
        } else if (t1_kind == hir::BuiltinType::Int16) {
            if (t2_kind == hir::BuiltinType::UInt8) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt16) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt32) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::Int32, span);
            } else if (t2_kind == hir::BuiltinType::UInt64) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::Int64, span);
            } else if (t2_kind == hir::BuiltinType::USize) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::ISize, span);
            } else if (t2_kind == hir::BuiltinType::Int8) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int16) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int32) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int64) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::ISize) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            }
        } else if (t1_kind == hir::BuiltinType::Int32) {
            if (t2_kind == hir::BuiltinType::UInt8) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt16) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt32) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt64) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::Int64, span);
            } else if (t2_kind == hir::BuiltinType::USize) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::ISize, span);
            } else if (t2_kind == hir::BuiltinType::Int8) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int16) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int32) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int64) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            } else if (t2_kind == hir::BuiltinType::ISize) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            }
        } else if (t1_kind == hir::BuiltinType::Int64) {
            if (t2_kind == hir::BuiltinType::UInt8) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt16) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt32) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt64) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::USize) {
                return std::make_shared<hir::BuiltinType>(
                    hir::BuiltinType::ISize, span);
            } else if (t2_kind == hir::BuiltinType::Int8) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int16) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int32) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int64) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::ISize) {
                return std::make_shared<hir::BuiltinType>(t2_kind, span);
            }
        } else if (t1_kind == hir::BuiltinType::ISize) {
            if (t2_kind == hir::BuiltinType::UInt8) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt16) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt32) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::UInt64) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::USize) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int8) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int16) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int32) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::Int64) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            } else if (t2_kind == hir::BuiltinType::ISize) {
                return std::make_shared<hir::BuiltinType>(t1_kind, span);
            }
        } else if (t1_kind == hir::BuiltinType::Void ||
                   t1_kind == hir::BuiltinType::Char ||
                   t1_kind == hir::BuiltinType::Bool) {
            if (t1_kind == t2_kind) {
                return std::make_shared<hir::BuiltinType>(
                    t1_kind, t1->span() + t2->span());
            } else {
                goto failed;
            }
        }
    }

failed:
    ReportInfo info(t1->span() + t2->span(), "cannot merge two type implicitly",
                    "");
    Report(ctx.ctx(), ReportLevel::Error, info);
    return std::nullopt;
}

}  // namespace mini
