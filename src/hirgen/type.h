#ifndef MINI_HIRGEN_TYPE_H_
#define MINI_HIRGEN_TYPE_H_

#include "../ast/type.h"
#include "../hir/type.h"
#include "context.h"

namespace mini {

class TypeHirGen : public ast::TypeVisitor {
public:
    TypeHirGen(HirGenContext &ctx)
        : success_(false), type_(nullptr), ctx_(ctx) {}
    explicit operator bool() const { return success_; }
    const std::shared_ptr<hir::Type> &type() const { return type_; }
    void visit(const ast::BuiltinType &type) override;
    void visit(const ast::PointerType &type) override;
    void visit(const ast::ArrayType &type) override;
    void visit(const ast::NameType &type) override;

private:
    bool success_;
    std::shared_ptr<hir::Type> type_;
    HirGenContext &ctx_;
};

}  // namespace mini

#endif  // MINI_HIRGEN_TYPE_H_
