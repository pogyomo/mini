#ifndef MINI_AST_DECL_H_
#define MINI_AST_DECL_H_

#include <string>

#include "node.h"
#include "stmt.h"
#include "type.h"

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
    const std::string name_;
    const Span span_;
};

class FunctionDeclarationParameterName : public Node {
public:
    FunctionDeclarationParameterName(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    const std::string name_;
    const Span span_;
};

class FunctionDeclarationParameter : public Node {
public:
    FunctionDeclarationParameter(FunctionDeclarationParameterName&& name,
                                 Colon colon, std::unique_ptr<Type>&& type)
        : name_(std::move(name)), colon_(colon), type_(std::move(type)) {}
    inline Span span() const override { return type_->span() + name_.span(); }
    inline const std::unique_ptr<Type>& type() const { return type_; }
    inline Colon colon() const { return colon_; }
    inline const FunctionDeclarationParameterName& name() const {
        return name_;
    }

private:
    FunctionDeclarationParameterName name_;
    Colon colon_;
    std::unique_ptr<Type> type_;
};

class FunctionDeclaration : public Declaration {
public:
    FunctionDeclaration(Function function_kw, FunctionDeclarationName&& name,
                        LParen lparen,
                        std::vector<FunctionDeclarationParameter>&& params,
                        RParen rparen, Arrow arrow,
                        std::unique_ptr<Type>&& type,
                        std::unique_ptr<BlockStatement>&& body)
        : function_kw_(function_kw),
          name_(std::move(name)),
          lparen_(lparen),
          params_(std::move(params)),
          rparen_(rparen),
          arrow_(arrow),
          type_(std::move(type)),
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
    inline Arrow arrow() const { return arrow_; }
    inline const std::unique_ptr<Type>& type() const { return type_; }
    inline const std::unique_ptr<BlockStatement>& body() const { return body_; }

private:
    const Function function_kw_;
    const FunctionDeclarationName name_;
    const LParen lparen_;
    const std::vector<FunctionDeclarationParameter> params_;
    const RParen rparen_;
    const Arrow arrow_;
    const std::unique_ptr<Type> type_;
    const std::unique_ptr<BlockStatement> body_;
};

class StructDeclarationName : public Node {
public:
    StructDeclarationName(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    const std::string name_;
    const Span span_;
};

class StructDeclarationFieldName : public Node {
public:
    StructDeclarationFieldName(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    const std::string name_;
    const Span span_;
};

class StructDeclarationField : public Node {
public:
    StructDeclarationField(StructDeclarationFieldName&& name, Colon colon,
                           std::unique_ptr<Type>&& type)
        : name_(std::move(name)), colon_(colon), type_(std::move(type)) {}
    inline Span span() const override { return type_->span() + name_.span(); }
    inline const std::unique_ptr<Type>& type() const { return type_; }
    inline const StructDeclarationFieldName& name() const { return name_; }

private:
    StructDeclarationFieldName name_;
    Colon colon_;
    std::unique_ptr<Type> type_;
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
    const Struct struct_kw_;
    const StructDeclarationName name_;
    const LCurly lcurly_;
    const std::vector<StructDeclarationField> fields_;
    const RCurly rcurly_;
};

class EnumDeclarationName : public Node {
public:
    EnumDeclarationName(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    const std::string name_;
    const Span span_;
};

class EnumDeclarationField : public Node {
public:
    EnumDeclarationField(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    const std::string name_;
    const Span span_;
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
    const Enum enum_kw_;
    const EnumDeclarationName name_;
    const LCurly lcurly_;
    const std::vector<EnumDeclarationField> fields_;
    const RCurly rcurly_;
};

#endif  // MINI_AST_DECL_H_
