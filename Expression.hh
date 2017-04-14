/**
 * @file Expression.hh
 *
 * @brief The classes comprising the expression portion of the AST.
 *
 * This is an AST header file, and thus follows the structure of the other AST
 * headers: there are many small class in this file, most with a few public
 * data members and a simple constructor taking corresponding arguments.
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
#include "Type.hh"

namespace Craeft {

namespace AST {

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

}

}
