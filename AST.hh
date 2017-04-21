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
 * @defgroup Types Classes for types as represented in the AST.
 *
 * @{
 */

/**
 * @brief A signed integer type.
 */
struct IntType {
    int nbits;

    IntType(int nbits): nbits(nbits) {}
};

/**
 * @brief An unsigned integer type.
 */
struct UIntType {
    int nbits;

    UIntType(int nbits): nbits(nbits) {}
};

/**
 * @brief A single-precision float.
 */
struct Float {};

/**
 * @brief A double-precision float.
 */
struct Double {};

/**
 * @brief The void type.
 */
struct Void {};

/**
 * @brief Any other type.
 */
struct UserType {
    std::string name;
    
    UserType(std::string n): name(n) {}
};

struct Pointer;

typedef boost::variant< IntType, UIntType, Float, Double, Void, UserType,
                        std::unique_ptr<Pointer> > Type;

/**
 * @brief A pointer to another type.
 */
struct Pointer {
    Type pointed;
};

/** @} */

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
 * @brief Variables.
 */
struct Variable {
    std::string name;
    SourcePos pos;

    Variable(std::string name, SourcePos pos)
        : name(std::move(name)), pos(pos) {}
};

/* Forward declarations. */
struct Binop;
struct FunctionCall;
struct Cast;

/**
 * @brief Expressions.
 *
 * Use move semantics.
 */
typedef boost::variant< IntLiteral,
                        UIntLiteral,
                        FloatLiteral,
                        Variable,
                        std::unique_ptr<Binop>,
                        std::unique_ptr<FunctionCall>,
                        std::unique_ptr<Cast> > Expression;

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

/* TODO: Expand to include dereferences of expressions. */
typedef boost::variant < Variable > LValue;

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

    Return(std::unique_ptr<Expression> retval): retval(std::move(retval)) {}
};

/**
 * @brief Variable declarations.
 */
struct Declaration {
    Type type;
    Variable name;

    Declaration(Type type, Variable name)
        : type(std::move(type)), name(name) {}
};

/**
 * @brief Compound variable declarations.
 *
 * I.e. Typename varname = expression;
 */
struct CompoundDeclaration {
    Type type;
    Variable name;
    Expression rhs;

    CompoundDeclaration(Type type, Variable name, Expression rhs)
        : type(std::move(type)), name(name), rhs(std::move(rhs)) {}
};

typedef boost::variant< std::unique_ptr<Expression>,
                        Return,
                        std::unique_ptr<Declaration>,
                        std::unique_ptr<CompoundDeclaration> >
    Statement;

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

    TypeDeclaration(std::string name): name(name) {}
};

/**
 * @brief Struct declarations.
 */
struct StructDeclaration {
    std::string name;
    std::vector<std::unique_ptr<Declaration>> members;

    StructDeclaration(std::string name,
                      std::vector<std::unique_ptr<Declaration> > members)
        : name(name), members(std::move(members)) {}
};

/**
 * @brief Function declarations.
 */
struct FunctionDeclaration {
    std::string name;
    std::vector<std::unique_ptr<Declaration> > args;
    Type ret_type;

    FunctionDeclaration(std::string name,
                        std::vector<std::unique_ptr<Declaration> > args,
                        Type ret_type)
        : name(std::move(name)), args(std::move(args)),
          ret_type(std::move(ret_type)) {}
};

/**
 * @brief Function definitions.
 */
struct FunctionDefinition {
    FunctionDeclaration signature;
    std::vector<Statement> block;

    FunctionDefinition(FunctionDeclaration signature,
                       std::vector<Statement> block)
        : signature(std::move(signature)), block(std::move(block)) {}
};

typedef boost::variant< TypeDeclaration, StructDeclaration,
                        FunctionDeclaration,
                        std::unique_ptr<FunctionDefinition> > TopLevel;

void print_toplevel(const TopLevel &, std::ostream &out);

/** @} */

}

}
