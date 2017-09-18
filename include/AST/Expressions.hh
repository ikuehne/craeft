/**
 * @file Expressions.hh
 *
 * @brief The AST nodes that represent expressions.
 */

/* Craeft: a new systems programming language.
 *
 * Copyright (C) 2017 Ian Kuehne <ikuehne@caltech.edu>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "llvm/Support/Casting.h"

#include "Error.hh"

namespace Craeft {

namespace AST {

class Type;

class Expression {
public:
    enum ExpressionKind {
        IntLiteral,
        UIntLiteral,
        FloatLiteral,
        StringLiteral,
        Variable,
        Reference,
        Dereference,
        FieldAccess,
        Binop,
        FunctionCall,
        TemplateFunctionCall,
        Cast
    };

    ExpressionKind kind(void) const {
        return _kind;
    }

    virtual ~Expression() {}
    SourcePos pos(void) const { return _pos; }

    void set_pos(SourcePos pos) { _pos = pos; }

    Expression(ExpressionKind kind, SourcePos pos): _kind(kind), _pos(pos) {}

private:
    ExpressionKind _kind;
    SourcePos _pos;
};

class LValue: public Expression {
public:
    static bool classof(const Expression *e) {
        return e->kind() == ExpressionKind::Variable
            || e->kind() == ExpressionKind::Dereference
            || e->kind() == ExpressionKind::FieldAccess;
    }

    LValue(ExpressionKind kind, SourcePos pos): Expression(kind, pos) {}
};

#define EXPRESSION_CLASS(X)\
    static bool classof(const Expression *e) {\
        return e->kind() == ExpressionKind::X;\
    }\
    ~X(void) override {}

#define LVALUE_CLASS(X)\
    static bool classof(const LValue *l) {\
        return l->kind() == ExpressionKind::X;\
    }\
    EXPRESSION_CLASS(X)


class IntLiteral: public Expression {
public:
    IntLiteral(int64_t value, SourcePos pos)
        : Expression(ExpressionKind::IntLiteral, pos), _value(value) {}

    int64_t value(void) const { return _value; }

    EXPRESSION_CLASS(IntLiteral);
private:
    int64_t _value;
};

class UIntLiteral: public Expression {
public:
    UIntLiteral(uint64_t value, SourcePos pos)
        : Expression(ExpressionKind::UIntLiteral, pos), _value(value) {}

    uint64_t value(void) const { return _value; }

    EXPRESSION_CLASS(UIntLiteral);
private:
    uint64_t _value;
};

class FloatLiteral: public Expression {
public:
    FloatLiteral(double value, SourcePos pos)
        : Expression(ExpressionKind::FloatLiteral, pos), _value(value) {}

    double value(void) const { return _value; }

    EXPRESSION_CLASS(FloatLiteral);
private:
    double _value;
};

class StringLiteral: public Expression {
public:
    StringLiteral(const std::string &value, SourcePos pos)
        : Expression(ExpressionKind::StringLiteral, pos), _value(value) {}

    const std::string &value(void) const { return _value; }

    EXPRESSION_CLASS(StringLiteral);
private:
    std::string _value;
};

class Variable: public LValue {
public:
    Variable(const std::string &name, SourcePos pos)
        : LValue(ExpressionKind::Variable, pos),
          _name(name) {}

    const std::string &name(void) const { return _name; }

    LVALUE_CLASS(Variable);
private:
    std::string _name;
};

class Reference: public Expression {
public:
    Reference(std::unique_ptr<LValue> referand, SourcePos pos)
        : Expression(ExpressionKind::Reference, pos),
          _referand(std::move(referand)) {}

    const LValue &referand(void) const { return *_referand; }

    EXPRESSION_CLASS(Reference);

private:
    std::unique_ptr<LValue> _referand;
};

class Dereference: public LValue {
public:
    Dereference(std::unique_ptr<Expression> referand, SourcePos pos)
        : LValue(ExpressionKind::Dereference, pos),
          _referand(std::move(referand)) {}

    const Expression &referand(void) const { return *_referand; }

    LVALUE_CLASS(Dereference);
private:
    std::unique_ptr<Expression> _referand;
};

class Binop: public Expression {
public:
    Binop(const std::string &op,
          std::unique_ptr<Expression> lhs,
          std::unique_ptr<Expression> rhs,
          SourcePos pos)
        : Expression(ExpressionKind::Binop, pos),
          _op(op), 
          _lhs(std::move(lhs)),
          _rhs(std::move(rhs)) {}

    const std::string &op(void) const { return _op; }

    const Expression &lhs(void) const { return *_lhs; }
    const Expression &rhs(void) const { return *_rhs; }

    std::unique_ptr<Expression> release_lhs(void) {
        std::unique_ptr<Expression> result;
        std::swap(_lhs, result);
        return result;
    }

    std::unique_ptr<Expression> release_rhs(void) {
        std::unique_ptr<Expression> result;
        std::swap(_rhs, result);
        return result;
    }

    EXPRESSION_CLASS(Binop);
private:
    std::string _op;
    std::unique_ptr<Expression> _lhs;
    std::unique_ptr<Expression> _rhs;
};

class FunctionCall: public Expression {
public:
    FunctionCall(const std::string &fname,
                 std::vector<std::unique_ptr<Expression>> args,
                 SourcePos pos)
        : Expression(ExpressionKind::FunctionCall, pos),
          _fname(fname),
          _args(std::move(args)) {}

    const std::string &fname(void) const { return _fname; }
    const std::vector<std::unique_ptr<Expression>> &args(void) const {
        return _args;
    }

    EXPRESSION_CLASS(FunctionCall);
private:
    std::string _fname;
    std::vector<std::unique_ptr<Expression>> _args;
};

class TemplateFunctionCall: public Expression {
public:
    TemplateFunctionCall(const std::string &fname,
                         std::vector<std::unique_ptr<Type>> type_args,
                         std::vector<std::unique_ptr<Expression>> value_args,
                         SourcePos pos)
        : Expression(ExpressionKind::TemplateFunctionCall, pos),
          _fname(fname),
          _type_args(std::move(type_args)),
          _value_args(std::move(value_args)) {}

    const std::string &fname(void) const { return _fname; }

    const std::vector<std::unique_ptr<Expression>> &value_args(void) const {
        return _value_args;
    }

    const std::vector<std::unique_ptr<Type>> &type_args(void) const {
        return _type_args;
    }

    EXPRESSION_CLASS(TemplateFunctionCall);
private:
    std::string _fname;
    std::vector<std::unique_ptr<Type>> _type_args;
    std::vector<std::unique_ptr<Expression>> _value_args;
};

class Cast: public Expression {
public:
    Cast(std::unique_ptr<Type> type,
         std::unique_ptr<Expression> arg,
         SourcePos pos)
        : Expression(ExpressionKind::Cast, pos),
          _type(std::move(type)),
          _arg(std::move(arg)) {} 

    const Type &type(void) const { return *_type; }
    const Expression &arg(void) const { return *_arg; }

    EXPRESSION_CLASS(Cast);
private:
    std::unique_ptr<Type> _type;
    std::unique_ptr<Expression> _arg;
};

class FieldAccess: public LValue {
public:
    FieldAccess(std::unique_ptr<Expression> structure,
                const std::string &field,
                SourcePos pos)
        : LValue(ExpressionKind::FieldAccess, pos),
          _structure(std::move(structure)),
          _field(field) {}

    const Expression &structure(void) const { return *_structure; }
    const std::string &field(void) const { return _field; }

    LVALUE_CLASS(FieldAccess);

private:
    std::unique_ptr<Expression> _structure;
    std::string _field;
};

#undef EXPRESSION_CLASS
#undef LVALUE_CLASS

template<typename Result>
class ExpressionVisitor {
public:
    virtual ~ExpressionVisitor() {};

    Result visit(const Expression &expr) {
        switch (expr.kind()) {
#define HANDLE(X) case Expression::ExpressionKind::X:\
                      return operator()(llvm::cast<X>(expr));
            HANDLE(IntLiteral);
            HANDLE(UIntLiteral);
            HANDLE(FloatLiteral);
            HANDLE(StringLiteral);
            HANDLE(Variable);
            HANDLE(Reference);
            HANDLE(Dereference);
            HANDLE(FieldAccess);
            HANDLE(Binop);
            HANDLE(FunctionCall);
            HANDLE(TemplateFunctionCall);
            HANDLE(Cast);
#undef HANDLE
        }
    }
private:
    virtual Result operator()(const IntLiteral &) = 0;
    virtual Result operator()(const UIntLiteral &) = 0;
    virtual Result operator()(const FloatLiteral &) = 0;
    virtual Result operator()(const StringLiteral &) = 0;
    virtual Result operator()(const Variable &) = 0;
    virtual Result operator()(const Reference &) = 0;
    virtual Result operator()(const Dereference &) = 0;
    virtual Result operator()(const FieldAccess &) = 0;
    virtual Result operator()(const Binop &) = 0;
    virtual Result operator()(const FunctionCall &) = 0;
    virtual Result operator()(const TemplateFunctionCall &) = 0;
    virtual Result operator()(const Cast &) = 0;
};

template<typename Result>
class ConsumingExpressionVisitor {
public:
    virtual ~ConsumingExpressionVisitor() {};

    Result visit(std::unique_ptr<Expression> expr) {
        switch (expr->kind()) {
#define HANDLE(X) case Expression::ExpressionKind::X: {\
                      X *_HANDLE_PTR_ = llvm::cast<X>(expr.release());\
                      std::unique_ptr<X> _HANDLE_RESULT_(_HANDLE_PTR_);\
                      return operator()(std::move(_HANDLE_RESULT_));\
                  }
            HANDLE(IntLiteral);
            HANDLE(UIntLiteral);
            HANDLE(FloatLiteral);
            HANDLE(StringLiteral);
            HANDLE(Variable);
            HANDLE(Reference);
            HANDLE(Dereference);
            HANDLE(FieldAccess);
            HANDLE(Binop);
            HANDLE(FunctionCall);
            HANDLE(TemplateFunctionCall);
            HANDLE(Cast);
#undef HANDLE
        }
    }

private:
    virtual Result operator()(std::unique_ptr<IntLiteral>) = 0;
    virtual Result operator()(std::unique_ptr<UIntLiteral>) = 0;
    virtual Result operator()(std::unique_ptr<FloatLiteral>) = 0;
    virtual Result operator()(std::unique_ptr<StringLiteral>) = 0;
    virtual Result operator()(std::unique_ptr<Variable>) = 0;
    virtual Result operator()(std::unique_ptr<Reference>) = 0;
    virtual Result operator()(std::unique_ptr<Dereference>) = 0;
    virtual Result operator()(std::unique_ptr<FieldAccess>) = 0;
    virtual Result operator()(std::unique_ptr<Binop>) = 0;
    virtual Result operator()(std::unique_ptr<FunctionCall>) = 0;
    virtual Result operator()(std::unique_ptr<TemplateFunctionCall>) = 0;
    virtual Result operator()(std::unique_ptr<Cast>) = 0;
};

template<typename Result>
class LValueVisitor {
public:
    virtual ~LValueVisitor() {};

    Result visit(const LValue &lvalue) {
        switch (lvalue.kind()) {
#define HANDLE(X) case LValue::ExpressionKind::X:\
                      return operator()(llvm::cast<X>(lvalue));
            HANDLE(Variable);
            HANDLE(Dereference);
            HANDLE(FieldAccess);
#undef HANDLE
            default:
                assert(false);
        }
    }

private:
    virtual Result operator()(const Variable &) = 0;
    virtual Result operator()(const Dereference &) = 0;
    virtual Result operator()(const FieldAccess &) = 0;
};

}

}

