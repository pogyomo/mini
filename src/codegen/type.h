#ifndef MINI_CODEGEN_TYPE_H_
#define MINI_CODEGEN_TYPE_H_

#include <memory>
#include <optional>
#include <string>

#include "../hir/type.h"
#include "context.h"

namespace mini {

class TypeAlignCalc : public hir::TypeVisitor {
public:
    TypeAlignCalc(CodeGenContext &ctx) : success_(false), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    uint64_t align() const { return align_; }
    void Visit(const hir::BuiltinType &type) override;
    void Visit(const hir::PointerType &type) override;
    void Visit(const hir::ArrayType &type) override;
    void Visit(const hir::NameType &type) override;

private:
    bool success_;
    uint64_t align_;
    CodeGenContext &ctx_;
};

class TypeSizeCalc : public hir::TypeVisitor {
public:
    TypeSizeCalc(CodeGenContext &ctx) : success_(false), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    uint64_t size() const { return size_; }
    void Visit(const hir::BuiltinType &type) override;
    void Visit(const hir::PointerType &type) override;
    void Visit(const hir::ArrayType &type) override;
    void Visit(const hir::NameType &type) override;

private:
    bool success_;
    uint64_t size_;
    CodeGenContext &ctx_;
};

// Calculate struct size and offsets of the name `name` and save it to table
// entry.
bool CalculateStructSizeAndOffset(CodeGenContext &ctx, const std::string &name,
                                  Span span);

// Merge two types so each type can be implicitly converted into merged one.
std::optional<std::shared_ptr<hir::Type>> ImplicitlyMergeTwoType(
    CodeGenContext &ctx, const std::shared_ptr<hir::Type> &t1,
    const std::shared_ptr<hir::Type> &t2);

}  // namespace mini

#endif  // MINI_CODEGEN_TYPE_H_
