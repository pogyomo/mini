#ifndef MINI_AST_DECL_H_
#define MINI_AST_DECL_H_

#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "expr.h"
#include "node.h"
#include "stmt.h"
#include "type.h"

namespace mini {

namespace ast {

class FunctionDeclaration;
class StructDeclaration;
class EnumDeclaration;

class DeclarationVisitor {
public:
    virtual ~DeclarationVisitor() {}
    virtual void Visit(const FunctionDeclaration& decl) = 0;
    virtual void Visit(const StructDeclaration& decl) = 0;
    virtual void Visit(const EnumDeclaration& decl) = 0;
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

class FunctionDeclarationParamName : public Node {
public:
    FunctionDeclarationParamName(std::string&& name, Span span)
        : name_(std::move(name)), span_(span) {}
    inline Span span() const override { return span_; }
    inline const std::string& name() const { return name_; }

private:
    std::string name_;
    Span span_;
};

class FunctionDeclarationParam : public Node {
public:
    FunctionDeclarationParam(FunctionDeclarationParamName&& name, Colon colon,
                             const std::shared_ptr<Type>& type)
        : name_(std::move(name)), colon_(colon), type_(type) {}
    inline Span span() const override { return type_->span() + name_.span(); }
    inline const std::shared_ptr<Type>& type() const { return type_; }
    inline Colon colon() const { return colon_; }
    inline const FunctionDeclarationParamName& name() const { return name_; }

private:
    FunctionDeclarationParamName name_;
    Colon colon_;
    std::shared_ptr<Type> type_;
};

class FunctionDeclarationReturn : public Node {
public:
    FunctionDeclarationReturn(Arrow arrow, const std::shared_ptr<Type>& type)
        : arrow_(arrow), type_(type) {}
    inline Span span() const override { return arrow_.span() + type_->span(); }
    inline Arrow arrow() const { return arrow_; }
    inline const std::shared_ptr<Type>& type() const { return type_; }

private:
    Arrow arrow_;
    std::shared_ptr<Type> type_;
};

class FunctionDeclarationVariadic : public Node {
public:
    FunctionDeclarationVariadic(DotDotDot dotdotdot) : dotdotdot_(dotdotdot) {}
    inline Span span() const override { return dotdotdot_.span(); }
    inline DotDotDot dotdotdot() const { return dotdotdot_; }

private:
    DotDotDot dotdotdot_;
};

class FunctionDeclarationBody : public Node {
public:
    FunctionDeclarationBody(std::unique_ptr<BlockStatement>&& concrete)
        : vars_(std::move(concrete)) {}
    FunctionDeclarationBody(Semicolon opaque) : vars_(opaque) {}
    inline Span span() const {
        return vars_.index() == 0 ? std::get<0>(vars_)->span()
                                  : std::get<1>(vars_).span();
    }
    inline bool IsConcrete() const { return vars_.index() == 0; }
    inline bool IsOpaque() const { return vars_.index() == 1; }
    inline const std::unique_ptr<BlockStatement>& ToConcrete() const {
        return std::get<0>(vars_);
    }
    inline Semicolon ToOpaque() const { return std::get<1>(vars_); }

private:
    std::variant<std::unique_ptr<BlockStatement>, Semicolon> vars_;
};

class FunctionDeclaration : public Declaration {
public:
    FunctionDeclaration(Function function_kw, FunctionDeclarationName&& name,
                        LParen lparen,
                        std::vector<FunctionDeclarationParam>&& params,
                        std::optional<FunctionDeclarationVariadic> variadic,
                        RParen rparen,
                        std::optional<FunctionDeclarationReturn>&& ret,
                        FunctionDeclarationBody&& body)
        : function_kw_(function_kw),
          name_(std::move(name)),
          lparen_(lparen),
          params_(std::move(params)),
          variadic_(variadic),
          rparen_(rparen),
          ret_(std::move(ret)),
          body_(std::move(body)) {}
    inline void Accept(DeclarationVisitor& visitor) const override {
        visitor.Visit(*this);
    }
    inline Span span() const override {
        return function_kw_.span() + body_.span();
    }
    inline Function function_kw() const { return function_kw_; }
    inline const FunctionDeclarationName& name() const { return name_; }
    inline std::optional<FunctionDeclarationVariadic> variadic() const {
        return variadic_;
    }
    inline LParen lparen() const { return lparen_; }
    inline const std::vector<FunctionDeclarationParam>& params() const {
        return params_;
    }
    inline RParen rparen() const { return rparen_; }
    const std::optional<FunctionDeclarationReturn>& ret() const { return ret_; }
    inline const FunctionDeclarationBody& body() const { return body_; }

private:
    Function function_kw_;
    FunctionDeclarationName name_;
    LParen lparen_;
    std::vector<FunctionDeclarationParam> params_;
    std::optional<FunctionDeclarationVariadic> variadic_;
    RParen rparen_;
    std::optional<FunctionDeclarationReturn> ret_;
    FunctionDeclarationBody body_;
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

class EnumBaseType : public Node {
public:
    EnumBaseType(Colon colon, std::unique_ptr<Type>&& type)
        : colon_(colon), type_(std::move(type)) {}
    inline Span span() const override { return colon_.span() + type_->span(); }
    inline Colon name() const { return colon_; }
    inline const std::shared_ptr<Type> type() const { return type_; }

private:
    Colon colon_;
    std::shared_ptr<Type> type_;
};

class EnumDeclaration : public Declaration {
public:
    EnumDeclaration(Enum enum_kw, EnumDeclarationName&& name,
                    std::optional<EnumBaseType>&& base_type, LCurly lcurly,
                    std::vector<EnumDeclarationField>&& fields, RCurly rcurly)
        : enum_kw_(enum_kw),
          name_(std::move(name)),
          base_type_(std::move(base_type)),
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
    inline const std::optional<EnumBaseType>& base_type() const {
        return base_type_;
    }
    inline LCurly lcurly() const { return lcurly_; }
    inline const std::vector<EnumDeclarationField>& fields() const {
        return fields_;
    }
    inline RCurly rcurly() const { return rcurly_; }

private:
    Enum enum_kw_;
    EnumDeclarationName name_;
    std::optional<EnumBaseType> base_type_;
    LCurly lcurly_;
    std::vector<EnumDeclarationField> fields_;
    RCurly rcurly_;
};

};  // namespace ast

};  // namespace mini

#endif  // MINI_AST_DECL_H_
