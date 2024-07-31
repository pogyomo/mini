#ifndef MINI_HIR_DECL_H_
#define MINI_HIR_DECL_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../span.h"
#include "printable.h"
#include "stmt.h"
#include "type.h"

namespace mini {

namespace hir {

class StructDeclaration;
class EnumDeclaration;
class FunctionDeclaration;
class ImportDeclaration;

class DeclarationVisitor {
public:
    virtual ~DeclarationVisitor() {}
    virtual void Visit(const StructDeclaration &decl) = 0;
    virtual void Visit(const EnumDeclaration &decl) = 0;
    virtual void Visit(const FunctionDeclaration &decl) = 0;
    virtual void Visit(const ImportDeclaration &decl) = 0;
};

class Declaration : public Printable {
public:
    Declaration(Span span) : span_(span) {}
    virtual void Accept(DeclarationVisitor &visitor) const = 0;
    inline Span span() const { return span_; }

private:
    Span span_;
};

class StructDeclarationFieldName {
public:
    StructDeclarationFieldName(std::string &&value, Span span)
        : value_(std::move(value)), span_(span) {}
    inline const std::string &value() const { return value_; }
    inline Span span() const { return span_; }

private:
    std::string value_;
    Span span_;
};

class StructDeclarationField {
public:
    StructDeclarationField(const std::shared_ptr<Type> &type,
                           StructDeclarationFieldName &&name, Span span)
        : type_(std::move(type)), name_(std::move(name)), span_(span) {}
    inline const std::shared_ptr<Type> &type() const { return type_; }
    inline const StructDeclarationFieldName &name() const { return name_; }
    inline Span span() const { return span_; }

private:
    std::shared_ptr<Type> type_;
    StructDeclarationFieldName name_;
    Span span_;
};

class StructDeclarationName {
public:
    StructDeclarationName(std::string &&value, Span span)
        : value_(std::move(value)), span_(span) {}
    inline const std::string &value() const { return value_; }
    inline Span span() const { return span_; }

private:
    std::string value_;
    Span span_;
};

class StructDeclaration : public Declaration {
public:
    StructDeclaration(StructDeclarationName &&name,
                      std::vector<StructDeclarationField> &&fields, Span span)
        : Declaration(span),
          name_(std::move(name)),
          fields_(std::move(fields)) {}
    inline void Accept(DeclarationVisitor &visitor) const override {
        visitor.Visit(*this);
    }
    void Print(PrintableContext &ctx) const override;
    inline const StructDeclarationName &name() const { return name_; }
    inline const std::vector<StructDeclarationField> &fields() const {
        return fields_;
    }

private:
    StructDeclarationName name_;
    std::vector<StructDeclarationField> fields_;
};

class EnumDeclarationFieldName {
public:
    EnumDeclarationFieldName(std::string &&value, Span span)
        : value_(std::move(value)), span_(span) {}
    inline const std::string &value() const { return value_; }
    inline Span span() const { return span_; }

private:
    std::string value_;
    Span span_;
};

class EnumDeclarationFieldValue {
public:
    EnumDeclarationFieldValue(uint64_t value, Span span)
        : value_(value), span_(span) {}
    inline uint64_t value() const { return value_; }
    inline Span span() const { return span_; }

private:
    uint64_t value_;
    Span span_;
};

class EnumDeclarationField {
public:
    EnumDeclarationField(EnumDeclarationFieldName &&name,
                         EnumDeclarationFieldValue value)
        : name_(std::move(name)), value_(value) {}
    inline Span span() const { return name_.span() + value_.span(); }
    inline const EnumDeclarationFieldName &name() const { return name_; }
    inline EnumDeclarationFieldValue value() const { return value_; }

private:
    EnumDeclarationFieldName name_;
    EnumDeclarationFieldValue value_;
};

class EnumDeclarationName {
public:
    EnumDeclarationName(std::string &&value, Span span)
        : value_(std::move(value)), span_(span) {}
    inline const std::string &value() const { return value_; }
    inline Span span() const { return span_; }

private:
    std::string value_;
    Span span_;
};

class EnumDeclaration : public Declaration {
public:
    EnumDeclaration(EnumDeclarationName &&name,
                    std::vector<EnumDeclarationField> &&fields, Span span)
        : Declaration(span),
          name_(std::move(name)),
          fields_(std::move(fields)) {}
    inline void Accept(DeclarationVisitor &visitor) const override {
        visitor.Visit(*this);
    }
    void Print(PrintableContext &ctx) const override;
    inline const EnumDeclarationName &name() const { return name_; }
    inline const std::vector<EnumDeclarationField> &fields() const {
        return fields_;
    }

private:
    EnumDeclarationName name_;
    std::vector<EnumDeclarationField> fields_;
};

class FunctionDeclarationName {
public:
    FunctionDeclarationName(std::string &&value, Span span)
        : value_(std::move(value)), span_(span) {}
    inline const std::string &value() const { return value_; }
    inline Span span() const { return span_; }

private:
    std::string value_;
    Span span_;
};

class FunctionDeclarationParamName {
public:
    FunctionDeclarationParamName(std::string &&value, Span span)
        : value_(std::move(value)), span_(span) {}
    inline const std::string &value() const { return value_; }
    inline Span span() const { return span_; }

private:
    std::string value_;
    Span span_;
};

class FunctionDeclarationParam {
public:
    FunctionDeclarationParam(const std::shared_ptr<Type> &type,
                             FunctionDeclarationParamName &&name, Span span)
        : type_(std::move(type)), name_(std::move(name)), span_(span) {}
    inline const std::shared_ptr<Type> &type() const { return type_; }
    inline const FunctionDeclarationParamName &name() const { return name_; }
    inline Span span() const { return span_; }

private:
    std::shared_ptr<Type> type_;
    FunctionDeclarationParamName name_;
    Span span_;
};

class VariableDeclarationName {
public:
    VariableDeclarationName(std::string &&value, Span span)
        : value_(std::move(value)), span_(span) {}
    inline const std::string &value() const { return value_; }
    inline Span span() const { return span_; }

private:
    std::string value_;
    Span span_;
};

class VariableDeclaration {
public:
    VariableDeclaration(const std::shared_ptr<Type> &type,
                        VariableDeclarationName &&name)
        : type_(type), name_(std::move(name)) {}
    inline const std::shared_ptr<Type> &type() const { return type_; }
    inline const VariableDeclarationName &name() const { return name_; }

private:
    std::shared_ptr<Type> type_;
    VariableDeclarationName name_;
};

class FunctionDeclaration : public Declaration {
public:
    FunctionDeclaration(FunctionDeclarationName &&name,
                        std::vector<FunctionDeclarationParam> &&params,
                        const std::shared_ptr<Type> &ret,
                        std::vector<VariableDeclaration> &&decls,
                        BlockStatement &&body, Span span)
        : Declaration(span),
          name_(std::move(name)),
          params_(std::move(params)),
          ret_(ret),
          decls_(std::move(decls)),
          body_(std::move(body)) {}
    inline void Accept(DeclarationVisitor &visitor) const override {
        visitor.Visit(*this);
    }
    void Print(PrintableContext &ctx) const override;
    inline const FunctionDeclarationName &name() const { return name_; }
    inline const std::vector<FunctionDeclarationParam> &params() const {
        return params_;
    }
    inline const std::shared_ptr<Type> &ret() const { return ret_; }
    inline const std::vector<VariableDeclaration> &decls() const {
        return decls_;
    }
    inline const BlockStatement &body() const { return body_; }

private:
    FunctionDeclarationName name_;
    std::vector<FunctionDeclarationParam> params_;
    std::shared_ptr<Type> ret_;
    std::vector<VariableDeclaration> decls_;
    BlockStatement body_;
};

class ImportDeclarationImportedItem {
public:
    ImportDeclarationImportedItem(std::string &&name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline const std::string &name() const { return name_; }
    inline Span span() const { return span_; }

private:
    std::string name_;
    Span span_;
};

class ImportDeclarationPathItem {
public:
    ImportDeclarationPathItem(std::string &&name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline const std::string &name() const { return name_; }
    inline Span span() const { return span_; }

private:
    std::string name_;
    Span span_;
};

class ImportDeclarationPath {
public:
    ImportDeclarationPath(ImportDeclarationPathItem &&head,
                          std::vector<ImportDeclarationPathItem> &&rest)
        : head_(std::move(head)), rest_(std::move(rest)) {}
    Span span() const {
        auto span = head_.span();
        for (const auto &item : rest_) span = span + item.span();
        return span;
    }
    inline const ImportDeclarationPathItem &head() const { return head_; }
    inline const std::vector<ImportDeclarationPathItem> &rest() const {
        return rest_;
    }

private:
    ImportDeclarationPathItem head_;
    std::vector<ImportDeclarationPathItem> rest_;
};

class ImportDeclaration : public Declaration {
public:
    ImportDeclaration(std::vector<ImportDeclarationImportedItem> &&items,
                      ImportDeclarationPath &&path, Span span)
        : Declaration(span), items_(std::move(items)), path_(std::move(path)) {}
    inline void Accept(DeclarationVisitor &visitor) const override {
        visitor.Visit(*this);
    }
    void Print(PrintableContext &ctx) const override;
    inline const std::vector<ImportDeclarationImportedItem> &items() const {
        return items_;
    }
    inline const ImportDeclarationPath &path() const { return path_; }

private:
    std::vector<ImportDeclarationImportedItem> items_;
    ImportDeclarationPath path_;
};

}  // namespace hir

}  // namespace mini

#endif  // MINI_HIR_DECL_H_
