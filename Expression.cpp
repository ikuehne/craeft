/**
 * @file Expression.cpp
 *
 * Implementations for operations on AST expression nodes.
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

#include "Expression.hh"

namespace Craeft {

namespace AST {

struct ExpressionPrintVisitor: boost::static_visitor<void> {
    ExpressionPrintVisitor(std::ostream &out): out(out) {}

    template<typename T>
    void operator()(const T &lit) {
        out << typeid(T).name() << " {" << lit.value << "}";
    }

    void operator()(const Variable &var) {
        out << "Variable {" << var.name << "}";
    }

    void operator()(const std::unique_ptr<Binop> &bin) {
        out << "Binop {" << bin->op << ", ";
        boost::apply_visitor(*this, bin->lhs);
        out << ", ";
        boost::apply_visitor(*this, bin->rhs);
        out << "}";
    }

    void operator()(const std::unique_ptr<FunctionCall> &fc) {
        out << "FunctionCall {" << fc->fname;
        for (const auto &arg: fc->args) {
            out << ", ";
            boost::apply_visitor(*this, arg);
        }
        out << "}";
    }

    void operator()(const std::unique_ptr<Cast> &c) {
        /* TODO: fix type rendering to handle non-user types. */
        out << "Cast {" << boost::get<UserType>(*c->t).name << ", ";
        boost::apply_visitor(*this, c->arg);
        out << "}";
    }

    std::ostream &out;
};

void print_expr(const Expression &expr, std::ostream &out) {
    auto printer = ExpressionPrintVisitor(out);
    boost::apply_visitor(printer, expr);
}

}

}
