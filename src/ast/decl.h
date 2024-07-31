#ifndef MINI_AST_DECL_H_
#define MINI_AST_DECL_H_

#include <optional>
#include <string>

#include "../panic.h"
#include "expr.h"
#include "node.h"
#include "stmt.h"
#include "type.h"

namespace mini {

namespace ast {

class FunctionDeclaration;
class StructDeclaration;
class EnumDeclaration;
class ImportDeclaration;

class DeclarationVisitor {
public:
    virtual ~DeclarationVisitor() {}
    virtual void Visit(const FunctionDeclaration& decl) = 0;
    virtual void Visit(const StructDeclaration& decl) = 0;
    virtual void Visit(const EnumDeclaration& decl) = 0;
    virtual void Visit(const ImportDeclaration& decl) = 0;
};

class Declaration : public Node {
public:
    virtual ~Declaration() {}
    virtual void Accept(DeclarationVisitor& visitor) const = 0;
};

class FunctionDeclarationName : public Node {
public:
    FunctionDeclarationName(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

class FunctionDeclarationParameterName : public Node {
public:
    FunctionDeclarationParameterName(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

class FunctionDeclarationParameter : public Node {
public:
    FunctionDeclarationParameter(FunctionDeclarationParameterName&& name,
                                 Colon colon, const std::shared_ptr<Type>& type)
        : name_(std::move(name)), colon_(colon), type_(type) {}
    inline Span span() const override { return type_->span() + name_.span(); }
    inline const std::shared_ptr<Type>& type() const { return type_; }
    inline Colon colon() const { return colon_; }
    inline const FunctionDeclarationParameterName& name() const {
        return name_;
    }

private:
    FunctionDeclarationParameterName name_;
    Colon colon_;
    std::shared_ptr<Type> type_;
};

class FunctionDeclarationReturn : public Node {
public:
    FunctionDeclarationReturn(Arrow arrow, const std::shared_ptr<Type>& type)
        : arrow_(arrow), type_(type) {}
    inline Span span() const { return arrow_.span() + type_->span(); }
    inline Arrow arrow() const { return arrow_; }
    inline const std::shared_ptr<Type>& type() const { return type_; }

private:
    Arrow arrow_;
    std::shared_ptr<Type> type_;
};

class FunctionDeclaration : public Declaration {
public:
    FunctionDeclaration(Function function_kw, FunctionDeclarationName&& name,
                        LParen lparen,
                        std::vector<FunctionDeclarationParameter>&& params,
                        RParen rparen,
                        std::optional<FunctionDeclarationReturn>&& ret,
                        std::unique_ptr<BlockStatement>&& body)
        : function_kw_(function_kw),
          name_(std::move(name)),
          lparen_(lparen),
          params_(std::move(params)),
          rparen_(rparen),
          ret_(std::move(ret)),
          body_(std::move(body)) {}
    inline void Accept(DeclarationVisitor& visitor) const override {
        visitor.Visit(*this);
    }
    inline Span span() const override {
        return function_kw_.span() + body_->span();
    }
    inline Function function_kw() const { return function_kw_; }
    inline const FunctionDeclarationName& name() const { return name_; }
    inline LParen lparen() const { return lparen_; }
    inline const std::vector<FunctionDeclarationParameter>& params() const {
        return params_;
    }
    inline RParen rparen() const { return rparen_; }
    const std::optional<FunctionDeclarationReturn>& ret() const { return ret_; }
    inline const std::unique_ptr<BlockStatement>& body() const { return body_; }

private:
    Function function_kw_;
    FunctionDeclarationName name_;
    LParen lparen_;
    std::vector<FunctionDeclarationParameter> params_;
    RParen rparen_;
    std::optional<FunctionDeclarationReturn> ret_;
    std::unique_ptr<BlockStatement> body_;
};

class StructDeclarationName : public Node {
public:
    StructDeclarationName(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

class StructDeclarationFieldName : public Node {
public:
    StructDeclarationFieldName(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

class StructDeclarationField : public Node {
public:
    StructDeclarationField(StructDeclarationFieldName&& name, Colon colon,
                           const std::shared_ptr<Type>& type)
        : name_(std::move(name)), colon_(colon), type_(type) {}
    inline Span span() const override { return type_->span() + name_.span(); }
    inline const std::shared_ptr<Type>& type() const { return type_; }
    inline const StructDeclarationFieldName& name() const { return name_; }

private:
    StructDeclarationFieldName name_;
    Colon colon_;
    std::shared_ptr<Type> type_;
};

class StructDeclaration : public Declaration {
public:
    StructDeclaration(Struct struct_kw, StructDeclarationName&& name,
                      LCurly lcurly,
                      std::vector<StructDeclarationField>&& fields,
                      RCurly rcurly)
        : struct_kw_(struct_kw),
          name_(std::move(name)),
          lcurly_(lcurly),
          fields_(std::move(fields)),
          rcurly_(rcurly) {}
    inline void Accept(DeclarationVisitor& visitor) const override {
        visitor.Visit(*this);
    }
    inline Span span() const override {
        return struct_kw_.span() + rcurly_.span();
    }
    inline Struct struct_kw() const { return struct_kw_; }
    inline const StructDeclarationName name() const { return name_; }
    inline LCurly lcurly() const { return lcurly_; }
    inline const std::vector<StructDeclarationField>& fields() const {
        return fields_;
    }
    inline RCurly rcurly() const { return rcurly_; }

private:
    Struct struct_kw_;
    StructDeclarationName name_;
    LCurly lcurly_;
    std::vector<StructDeclarationField> fields_;
    RCurly rcurly_;
};

class EnumDeclarationName : public Node {
public:
    EnumDeclarationName(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

class EnumDeclarationFieldName : public Node {
public:
    EnumDeclarationFieldName(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

class EnumDeclarationFieldInit : public Node {
public:
    EnumDeclarationFieldInit(Assign assign, std::unique_ptr<Expression>&& value)
        : assign_(assign), value_(std::move(value)) {}
    inline Span span() const override {
        return assign_.span() + value_->span();
    }
    inline Assign assign() const { return assign_; }
    inline const std::unique_ptr<Expression>& value() const { return value_; }

private:
    Assign assign_;
    std::unique_ptr<Expression> value_;
};

class EnumDeclarationField : public Node {
public:
    EnumDeclarationField(EnumDeclarationFieldName&& name,
                         std::optional<EnumDeclarationFieldInit>&& init)
        : name_(std::move(name)), init_(std::move(init)) {}
    inline Span span() const override {
        return init_ ? name_.span() + init_->span() : name_.span();
    }
    inline const EnumDeclarationFieldName& name() const { return name_; }
    inline const std::optional<EnumDeclarationFieldInit>& init() const {
        return init_;
    }

private:
    EnumDeclarationFieldName name_;
    std::optional<EnumDeclarationFieldInit> init_;
};

class EnumDeclaration : public Declaration {
public:
    EnumDeclaration(Enum enum_kw, EnumDeclarationName&& name, LCurly lcurly,
                    std::vector<EnumDeclarationField>&& fields, RCurly rcurly)
        : enum_kw_(enum_kw),
          name_(std::move(name)),
          lcurly_(lcurly),
          fields_(std::move(fields)),
          rcurly_(rcurly) {}
    inline void Accept(DeclarationVisitor& visitor) const override {
        visitor.Visit(*this);
    }
    inline Span span() const override {
        return enum_kw_.span() + rcurly_.span();
    }
    inline Enum enum_kw() const { return enum_kw_; }
    inline const EnumDeclarationName name() const { return name_; }
    inline LCurly lcurly() const { return lcurly_; }
    inline const std::vector<EnumDeclarationField>& fields() const {
        return fields_;
    }
    inline RCurly rcurly() const { return rcurly_; }

private:
    Enum enum_kw_;
    EnumDeclarationName name_;
    LCurly lcurly_;
    std::vector<EnumDeclarationField> fields_;
    RCurly rcurly_;
};

class ImportDeclarationImportedSingleItem;
class ImportDeclarationImportedMultiItems;

class ImportDeclarationImportedItem : public Node {
public:
    virtual bool IsSingleItem() const { return false; }
    virtual bool IsMultiItems() const { return false; }
    virtual ImportDeclarationImportedSingleItem* ToSingleItem() {
        return nullptr;
    }
    virtual const ImportDeclarationImportedSingleItem* ToSingleItem() const {
        return nullptr;
    }
    virtual ImportDeclarationImportedMultiItems* ToMultiItems() {
        return nullptr;
    }
    virtual const ImportDeclarationImportedMultiItems* ToMultiItems() const {
        return nullptr;
    }
};

class ImportDeclarationImportedSingleItem
    : public ImportDeclarationImportedItem {
public:
    ImportDeclarationImportedSingleItem(std::string&& item, Span span)
        : item_(std::move(item)), span_(span) {}
    bool IsSingleItem() const override { return true; }
    ImportDeclarationImportedSingleItem* ToSingleItem() override {
        return this;
    }
    const ImportDeclarationImportedSingleItem* ToSingleItem() const override {
        return this;
    }
    inline Span span() const override { return span_; }
    inline const std::string& item() const { return item_; }

private:
    std::string item_;
    Span span_;
};

class ImportDeclarationImportedMultiItems
    : public ImportDeclarationImportedItem {
public:
    ImportDeclarationImportedMultiItems(
        LCurly lcurly, std::vector<ImportDeclarationImportedSingleItem>&& items,
        RCurly rcurly)
        : lcurly_(lcurly), items_(std::move(items)), rcurly_(rcurly) {}
    bool IsMultiItems() const override { return true; }
    ImportDeclarationImportedMultiItems* ToMultiItems() override {
        return this;
    }
    const ImportDeclarationImportedMultiItems* ToMultiItems() const override {
        return this;
    }
    inline Span span() const override {
        return lcurly_.span() + rcurly_.span();
    }
    inline LCurly lcurly() const { return lcurly_; }
    inline const std::vector<ImportDeclarationImportedSingleItem>& items()
        const {
        return items_;
    }
    inline RCurly rcurly() const { return rcurly_; }

private:
    LCurly lcurly_;
    std::vector<ImportDeclarationImportedSingleItem> items_;
    RCurly rcurly_;
};

class ImportDeclarationPathItemName : public Node {
public:
    ImportDeclarationPathItemName(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

class ImportDeclarationPathItem : public Node {
public:
    ImportDeclarationPathItem(ImportDeclarationPathItemName&& name,
                              std::optional<Dot> dot)
        : name_(std::move(name)), dot_(dot) {}
    inline Span span() const override {
        return dot_ ? name_.span() + dot_->span() : name_.span();
    }
    inline const ImportDeclarationPathItemName& name() const { return name_; }
    inline std::optional<Dot> dot() const { return dot_; }

private:
    ImportDeclarationPathItemName name_;
    std::optional<Dot> dot_;
};

class ImportDeclarationPath : public Node {
public:
    ImportDeclarationPath(ImportDeclarationPathItem&& head,
                          std::vector<ImportDeclarationPathItem>&& rest)
        : head_(std::move(head)), rest_(std::move(rest)) {}
    Span span() const override {
        auto span = rest_.at(0).span();
        for (size_t i = 0; i < rest_.size(); i++) {
            span = span + rest_.at(i).span();
        }
        return span;
    }
    inline const ImportDeclarationPathItem& head() const { return head_; }
    inline const std::vector<ImportDeclarationPathItem>& rest() const {
        return rest_;
    }

private:
    ImportDeclarationPathItem head_;
    std::vector<ImportDeclarationPathItem> rest_;
};

class ImportDeclaration : public Declaration {
public:
    ImportDeclaration(Import import_kw,
                      std::unique_ptr<ImportDeclarationImportedItem>&& item,
                      From from_kw, ImportDeclarationPath&& path,
                      Semicolon semicolon)
        : import_kw_(import_kw),
          item_(std::move(item)),
          from_kw_(from_kw),
          path_(std::move(path)),
          semicolon_(semicolon) {}
    inline void Accept(DeclarationVisitor& visitor) const override {
        visitor.Visit(*this);
    }
    inline Span span() const override {
        return import_kw_.span() + path_.span();
    }
    inline Import import_kw() const { return import_kw_; }
    inline const std::unique_ptr<ImportDeclarationImportedItem>& item() const {
        return item_;
    }
    inline From from_kw() const { return from_kw_; }
    inline const ImportDeclarationPath& path() const { return path_; }
    inline Semicolon semicolon() const { return semicolon_; }

private:
    Import import_kw_;
    std::unique_ptr<ImportDeclarationImportedItem> item_;
    From from_kw_;
    ImportDeclarationPath path_;
    Semicolon semicolon_;
};

};  // namespace ast

};  // namespace mini

#endif  // MINI_AST_DECL_H_
