/**
 * @file AST/Expressions.cpp
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

#include "AST/Expressions.hh"
#include "AST/Types.hh"

#include <ostream>

namespace Craeft {

namespace AST {

namespace {

/**
 * @brief Visitor for printing `AST::Expression`s.
 */
class ExpressionPrintVisitor: public ExpressionVisitor<void> {
public:
    ExpressionPrintVisitor(std::ostream &out): out(out) {}

private:
    void operator()(const IntLiteral &lit) override {
        out << "IntLiteral {" << lit.value() << "}";
    }

    void operator()(const UIntLiteral &lit) override {
        out << "UIntLiteral {" << lit.value() << "}";
    }

    void operator()(const FloatLiteral &lit) override {
        out << "FloatLiteral {" << lit.value() << "}";
    }

    void operator()(const StringLiteral &lit) override {
        out << "StringLiteral {" << lit.value() << "}";
    }

    void operator()(const Variable &var) override {
        out << "Variable {" << var.name() << "}";
    }

    void operator()(const Reference &ref) override {
        out << "Reference {";
        visit(ref.referand());
        out << "}";
    }

    void operator()(const Dereference &deref) override {
        out << "Dereference {";
        visit(deref.referand());
        out << "}";
    }

    void operator()(const FieldAccess &access) override {
        out << "FieldAccess {";
        visit(access.structure());
        out << ", " << access.field() << "}";
    }

    void operator()(const Binop &bin) override {
        out << "Binop {" << bin.op() << ", ";
        visit(bin.lhs());
        out << ", ";
        visit(bin.rhs());
        out << "}";
    }

    void operator()(const FunctionCall &fc) override {
        out << "FunctionCall {" << fc.fname();
        for (const auto &arg: fc.args()) {
            out << ", ";
            visit(*arg);
        }
        out << "}";
    }

    void operator()(const TemplateFunctionCall &fc) override {
        out << "TemplateFunctionCall {" << fc.fname() << ", ";

        out << "TemplateArgs {";

        if (fc.type_args().size()) {
            print_type(*fc.type_args()[0], out);
        }

        for (unsigned i = 1; i < fc.type_args().size(); ++i) {
            out << ", ";
            print_type(*fc.type_args()[i], out);
        }

        for (const auto &arg: fc.value_args()) {
            out << ", ";
            visit(*arg);
        }

        out << "}";
    }

    void operator()(const Cast &c) override {
        out << "Cast {";
        print_type(c.type(), out);
        out << ", ";
        visit(c.arg());
        out << "}";
    }

    std::ostream &out;
};

}

void print_expr(const Expression &expr, std::ostream &out) {
    ExpressionPrintVisitor printer(out);
    printer.visit(expr);
}

}

}
