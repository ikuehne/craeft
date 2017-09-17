/**
 * @file AST.hh
 *
 * @brief The classes comprising the abstract syntax tree.
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

#include <ostream>
#include <vector>

#include <boost/variant.hpp>

#include "ASTTypes.hh"
#include "Error.hh"

/* Architectural note: leaves of the AST are represented as small structs,
 * with a few public data members and a constructor taking corresponding
 * arguments.  Subtrees (for example, Expression) are boost::variants of those
 * other AST classes.
 *
 * Any of the AST classes which itself contains a potentially large AST class
 * should be held only as a std::unique_ptr, and AST classes that are not
 * known to be leaves should never be copied.
 */

namespace Craeft {

/**
 * @brief Contains all classes and utilities relating to the abstract syntax
 * tree.
 */
namespace AST {

/**
 * @defgroup Expressions Classes for expressions as represented in the AST.
 *
 * @{
 */

/**
 * @brief Signed integer literals.
 */
struct IntLiteral {
    int64_t value;
    SourcePos pos;

    IntLiteral(int64_t value, SourcePos pos): value(value), pos(pos) {}
};

/**
 * @brief Unsigned integer literals.
 */
struct UIntLiteral {
    uint64_t value;
    SourcePos pos;

    UIntLiteral(uint64_t value, SourcePos pos): value(value), pos(pos) {}
};

/**
 * @brief Floating-point literals.
 */
struct FloatLiteral {
    double value;
    SourcePos pos;

    FloatLiteral(double value, SourcePos pos): value(value), pos(pos) {}
};

/**
 * @brief String literals.
 */
struct StringLiteral {
    std::string value;
    SourcePos pos;

    StringLiteral(const std::string &value, SourcePos pos)
        : value(value), pos(pos) {}
};

/**
 * @brief Variables.
 */
struct Variable {
    std::string name;
    SourcePos pos;

    Variable(std::string name, SourcePos pos)
        : name(std::move(name)), pos(pos) {}
};

/* Forward declarations. */
struct Dereference;
struct Reference;
struct Binop;
struct FunctionCall;
struct TemplateFunctionCall;
struct Cast;

/**
 * @brief Expressions.
 *
 * Use move semantics.
 */
typedef boost::variant< IntLiteral,
                        UIntLiteral,
                        FloatLiteral,
                        StringLiteral,
                        Variable,
                        std::unique_ptr<Reference>,
                        std::unique_ptr<Dereference>,
                        std::unique_ptr<Binop>,
                        std::unique_ptr<FunctionCall>,
                        std::unique_ptr<TemplateFunctionCall>,
                        std::unique_ptr<Cast> > Expression;

/**
 * @brief Application of the dereference operator "*".
 */
struct Dereference {
    /**
     * @brief Expression being dereferenced.
     */
    Expression referand;
    SourcePos pos;

    Dereference(Expression referand, SourcePos pos)
        : referand(std::move(referand)), pos(pos) {}
};


/**
 * @brief Binary operator application.
 */
struct Binop {
    std::string op;

    Expression lhs;
    Expression rhs;

    SourcePos pos;

    Binop(std::string op, Expression lhs, Expression rhs, SourcePos pos)
        : op(std::move(op)),
          lhs(std::move(lhs)), rhs(std::move(rhs)),
          pos(pos) {}
};

/**
 * @brief Function calls.
 */
struct FunctionCall {
    std::string fname;

    std::vector<Expression> args;

    SourcePos pos;

    FunctionCall(std::string fname,
                 std::vector<Expression> args,
                 SourcePos pos)
        : fname(std::move(fname)), args(std::move(args)), pos(pos) {}
};

/**
 * @brief Calls to templated function calls with type arguments.
 */
struct TemplateFunctionCall {
    std::string fname;

    std::vector<std::unique_ptr<Type>> tmpl_args;
    std::vector<Expression> val_args;

    SourcePos pos;

    TemplateFunctionCall(std::string fname,
                         std::vector<std::unique_ptr<Type>> tmpl_args,
                         std::vector<Expression> val_args,
                         SourcePos pos)
        : fname(fname), tmpl_args(std::move(tmpl_args)),
          val_args(std::move(val_args)), pos(pos) {}
};

/**
 * @brief Casts, from the syntactic form (Typename)expression.
 */
struct Cast {
    std::unique_ptr<Type> t;

    Expression arg;

    SourcePos pos;

    Cast(std::unique_ptr<Type> t, Expression arg, SourcePos pos)
        : t(std::move(t)), arg(std::move(arg)), pos(pos) {}
};

/**
 * @brief Print a representation of the expression to the given stream.
 *
 * Intended for debugging.
 */
void print_expr(const Expression &, std::ostream &);

/** @} */

/**
 * @defgroup LValues Classes for l-values as represented in the AST.
 *
 * @{
 */

struct FieldAccess;
/* TODO: Expand to include arrays, etc. */
typedef boost::variant < Variable,
                         std::unique_ptr<Dereference>,
                         std::unique_ptr<FieldAccess> > LValue;

struct FieldAccess {
    LValue structure;
    std::string field;

    SourcePos pos;

    FieldAccess(LValue structure, std::string field, SourcePos pos)
        : structure(std::move(structure)), field(field), pos(pos) {}
};

/**
 * @brief Application of the address-of operator.
 */
struct Reference {
    /**
     * @brief L-value having its address taken.
     */
    LValue referand;
    SourcePos pos;

    Reference(LValue referand, SourcePos pos)
        : referand(std::move(referand)), pos(pos) {}
};

/** @} */

/**
 * @defgroup Statements Classes for statements as represented in the AST.
 *
 * @{
 */

/**
 * @brief Return statements.
 */
struct Return {
    std::unique_ptr<Expression> retval;

    SourcePos pos;

    Return(std::unique_ptr<Expression> retval, SourcePos pos)
        : retval(std::move(retval)), pos(pos) {}
};

/**
 * @brief Empty return statements.
 */
struct VoidReturn {
    SourcePos pos;

    VoidReturn(SourcePos pos): pos(pos) {}
};

/**
 * @brief Variable declarations.
 */
struct Declaration {
    std::unique_ptr<Type> type;
    Variable name;

    SourcePos pos;

    Declaration(std::unique_ptr<Type> type, Variable name, SourcePos pos)
        : type(std::move(type)), name(name), pos(pos) {}
};

/**
 * @brief Assignments.
 */
struct Assignment {
    LValue lhs;
    Expression rhs;

    SourcePos pos;

    Assignment(LValue lhs, Expression rhs, SourcePos pos)
        : lhs(std::move(lhs)), rhs(std::move(rhs)), pos(pos) {}
};

/**
 * @brief Compound variable declarations.
 *
 * I.e. Typename varname = expression;
 */
struct CompoundDeclaration {
    std::unique_ptr<Type> type;
    Variable name;
    Expression rhs;

    SourcePos pos;

    CompoundDeclaration(std::unique_ptr<Type> type,
                        Variable name,
                        Expression rhs,
                        SourcePos pos)
        : type(std::move(type)), name(name),
          rhs(std::move(rhs)), pos(pos) {}
};

struct IfStatement;

typedef boost::variant< Expression,
                        Return,
                        VoidReturn,
                        std::unique_ptr<Assignment>,
                        std::unique_ptr<Declaration>,
                        std::unique_ptr<CompoundDeclaration>,
                        std::unique_ptr<IfStatement> >
    Statement;

struct IfStatement {
    Expression condition;
    std::vector<Statement> if_block;
    std::vector<Statement> else_block;

    SourcePos pos;

    IfStatement(Expression cond,
                std::vector<Statement> if_block,
                std::vector<Statement> else_block,
                SourcePos pos)
        : condition(std::move(cond)),
          if_block(std::move(if_block)),
          else_block(std::move(else_block)),
          pos(pos) {}
};

void print_statement(const Statement &, std::ostream &out);

/** @} */

/**
 * @defgroup Toplevel Forms that may be used at the source level.
 *
 * @{
 */

/**
 * @brief Type declaration.
 *
 * Forward declarations of types.
 */
struct TypeDeclaration {
    std::string name;

    SourcePos pos;

    TypeDeclaration(std::string name, SourcePos pos): name(name), pos(pos) {}
};

/**
 * @brief Struct declarations.
 */
struct StructDeclaration {
    std::string name;
    std::vector<std::unique_ptr<Declaration>> members;
    SourcePos pos;

    StructDeclaration(std::string name,
                      std::vector<std::unique_ptr<Declaration> > members,
                      SourcePos pos)
        : name(name), members(std::move(members)), pos(pos) {}
};

/**
 * @brief Template struct declarations.
 */
struct TemplateStructDeclaration {
    std::vector<std::string> argnames;
    
    StructDeclaration decl;

    TemplateStructDeclaration(
            std::string name,
            std::vector<std::string> argnames,
            std::vector<std::unique_ptr<Declaration> > members,
            SourcePos pos)
        : argnames(argnames),
          decl(name, std::move(members), pos) {}
};


/**
 * @brief Function declarations.
 */
struct FunctionDeclaration {
    std::string name;
    std::vector<std::unique_ptr<Declaration> > args;
    std::unique_ptr<Type> ret_type;

    SourcePos pos;

    FunctionDeclaration(std::string name,
                        std::vector<std::unique_ptr<Declaration> > args,
                        std::unique_ptr<Type> ret_type, SourcePos pos)
        : name(std::move(name)), args(std::move(args)),
          ret_type(std::move(ret_type)), pos(pos) {}
};

/**
 * @brief Function definitions.
 */
struct FunctionDefinition {
    FunctionDeclaration signature;
    std::vector<Statement> block;

    SourcePos pos;

    FunctionDefinition(FunctionDeclaration signature,
                       std::vector<Statement> block,
                       SourcePos pos)
        : signature(std::move(signature)), block(std::move(block)),
          pos(pos) {}
};

/**
 * @brief Template function definitions.
 */
struct TemplateFunctionDefinition {
    std::vector<std::string> argnames;
    std::shared_ptr<FunctionDefinition> def;

    TemplateFunctionDefinition(FunctionDeclaration signature,
                               std::vector<std::string> argnames,
                               std::vector<Statement> block,
                               SourcePos pos)
        : argnames(std::move(argnames)),
          def(new FunctionDefinition(std::move(signature),
                                     std::move(block), pos)) {}
};

typedef boost::variant< TypeDeclaration, StructDeclaration,
                        TemplateStructDeclaration,
                        FunctionDeclaration,
                        std::unique_ptr<FunctionDefinition>,
                        TemplateFunctionDefinition > TopLevel;

void print_toplevel(const TopLevel &, std::ostream &out);

/** @} */

}

}
