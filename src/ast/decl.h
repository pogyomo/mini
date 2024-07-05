#ifndef MINI_AST_DECL_H_
#define MINI_AST_DECL_H_

#include <optional>
#include <string>

#include "node.h"
#include "stmt.h"
#include "type.h"

namespace ast {

class FunctionDeclaration;
class StructDeclaration;
class EnumDeclaration;

class DeclarationVisitor {
public:
    virtual ~DeclarationVisitor() {}
    virtual void visit(const FunctionDeclaration& decl) = 0;
    virtual void visit(const StructDeclaration& decl) = 0;
    virtual void visit(const EnumDeclaration& decl) = 0;
};

class Declaration : public Node {
public:
    virtual ~Declaration() {}
    virtual void accept(DeclarationVisitor& visitor) const = 0;
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
    inline void accept(DeclarationVisitor& visitor) const override {
        visitor.visit(*this);
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
    inline void accept(DeclarationVisitor& visitor) const override {
        visitor.visit(*this);
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

class EnumDeclarationField : public Node {
public:
    EnumDeclarationField(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
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
    inline void accept(DeclarationVisitor& visitor) const override {
        visitor.visit(*this);
    }
    inline Span span() const override {
        return enum_kw_.span() + rcurly_.span();
    }
    inline Enum enum_kw() const { return enum_kw_; }
    inline const EnumDeclarationName name() const { return name_; }
    inline LCurly lcurly() const { return lcurly_; }
    inline const std::vector<EnumDeclarationField> fields() const {
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

};  // namespace ast

#endif  // MINI_AST_DECL_H_
