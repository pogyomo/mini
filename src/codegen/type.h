#ifndef MINI_CODEGEN_TYPE_H_
#define MINI_CODEGEN_TYPE_H_

#include <memory>
#include <ostream>

#include "../ast/expr.h"
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
    void visit(const IntLiteralType &type) override { size_ = 8; }
    void visit(const UIntLiteralType &type) override { size_ = 8; }
    void visit(const IntType &type) override { size_ = 8; }
    void visit(const UIntType &type) override { size_ = 8; }
    void visit(const BoolType &type) override { size_ = 1; }
    void visit(const CharType &type) override { size_ = 4; }
    void visit(const PointerType &type) override { size_ = 8; }
    void visit(const ArrayType &type) override;
    void visit(const NameType &type) override;

private:
    bool success_;
    uint64_t size_;
    Context &ctx_;
    std::ostream &os_;
};

class ExprTypeInferencer : public ExpressionVisitor {
public:
    ExprTypeInferencer(Context &ctx, std::ostream &os)
        : success_(false), inferred_(nullptr), ctx_(ctx), os_(os) {}
    bool success() const { return success_; }
    std::shared_ptr<Type> inferred() const { return inferred_; }
    void visit(const UnaryExpression &expr) override;
    void visit(const InfixExpression &expr) override;
    void visit(const IndexExpression &expr) override;
    void visit(const CallExpression &expr) override;
    void visit(const AccessExpression &expr) override;
    void visit(const CastExpression &expr) override;
    void visit(const ESizeofExpression &expr) override;
    void visit(const TSizeofExpression &expr) override;
    void visit(const EnumSelectExpression &expr) override;
    void visit(const VariableExpression &expr) override;
    void visit(const IntegerExpression &expr) override;
    void visit(const StringExpression &expr) override;
    void visit(const BoolExpression &expr) override;

private:
    bool success_;
    std::shared_ptr<Type> inferred_;
    Context &ctx_;
    std::ostream &os_;
};

#endif  // MINI_CODEGEN_TYPE_H_
