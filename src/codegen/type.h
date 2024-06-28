#ifndef MINI_CODEGEN_TYPE_H_
#define MINI_CODEGEN_TYPE_H_

#include "../context.h"

class SymbolTableEntrySizeCalculator : public SymbolTableEntryVisitor {
public:
    SymbolTableEntrySizeCalculator(Context &ctx, std::ostream &os)
        : success_(true), size_(0), ctx_(ctx), os_(os) {}
    bool success() const { return success_; }
    uint64_t size() const { return size_; }
    void visit(const VariableEntry &entry) override;
    void visit(const StructEntry &entry) override;
    void visit(const EnumEntry &entry) override { size_ = 8; }
    void visit(const FunctionEntry &entry) override { size_ = 8; }

private:
    bool success_;
    uint64_t size_;
    Context &ctx_;
    std::ostream &os_;
};

class TypeSizeCalculator : public TypeVisitor {
public:
    TypeSizeCalculator(Context &ctx, std::ostream &os)
        : success_(true), size_(0), ctx_(ctx), os_(os) {}
    bool success() const { return success_; }
    uint64_t size() const { return size_; }
    void visit(const IntType &type) override { size_ = 8; }
    void visit(const UIntType &type) override { size_ = 8; }
    void visit(const BoolType &type) override { size_ = 1; }
    void visit(const PointerType &type) override { size_ = 8; }
    void visit(const ArrayType &type) override;
    void visit(const NameType &type) override;

private:
    bool success_;
    uint64_t size_;
    Context &ctx_;
    std::ostream &os_;
};

#endif  // MINI_CODEGEN_TYPE_H_
