/**
 * @file AST/Statements.cpp
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

#include <ostream>

#include "AST/Statements.hh"

namespace Craeft {

namespace AST {

namespace {

struct StatementPrintVisitor: StatementVisitor<void> {
    StatementPrintVisitor(std::ostream &out): out(out) {}

private:
    void operator()(const AST::Assignment &assgnt) {
        out << "Assignment {";
        print_expr(assgnt.lhs(), out);
        out << ", ";
        print_expr(assgnt.rhs(), out);
        out << "}";
    }

    void operator()(const ExpressionStatement &expr) {
        out << "Statement {";

        print_expr(expr.expr(), out);

        out << "}";
    }

    void operator()(const Declaration &decl) {
        out << "Declaration {";

        print_type(decl.type(), out);

        out << ", ";

        print_expr(decl.name(), out);

        out << "}";
    }

    void operator()(const CompoundDeclaration &decl) {
        out << "Declaration {";
        print_type(decl.type(), out);
        out << ", ";
        print_expr(decl.name(), out);
        out << ", ";

        print_expr(decl.rhs(), out);

        out << "}";
    }

    void operator()(const Return &ret) {
        out << "Return {";

        print_expr(ret.retval(), out);

        out << "}";
    }

    void operator()(const VoidReturn &_) {
        out << "VoidReturn {}";
    }

    void operator()(const IfStatement &ifstmt) {
        out << "IfStatement {";

        print_expr(ifstmt.condition(), out);

        out << ", ";

        out << "IfTrue {";

        if (ifstmt.if_block().size()) {
            visit(**ifstmt.if_block().begin());
            for (auto iter = ifstmt.if_block().begin() + 1;
                 iter != ifstmt.if_block().end(); ++iter) {
                out << ", ";
                visit(**iter);
            }
        }

        out << "}, Else {";

        if (ifstmt.else_block().size()) {
            visit(**ifstmt.else_block().begin());
            for (auto iter = ifstmt.else_block().begin() + 1;
                 iter != ifstmt.else_block().end(); ++iter) {
                out << ", ";
                visit(**iter);
            }
        }

        out << "}}";
    }

    std::ostream &out;
};

}

void print_statement(const Statement &stmt, std::ostream &out) {
    StatementPrintVisitor printer(out);
    printer.visit(stmt);
}

}

}
