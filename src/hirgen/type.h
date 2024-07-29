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
    std::shared_ptr<hir::Type> &type() { return type_; }
    void Visit(const ast::BuiltinType &type) override;
    void Visit(const ast::PointerType &type) override;
    void Visit(const ast::ArrayType &type) override;
    void Visit(const ast::NameType &type) override;

private:
    bool success_;
    std::shared_ptr<hir::Type> type_;
    HirGenContext &ctx_;
};

}  // namespace mini

#endif  // MINI_HIRGEN_TYPE_H_
