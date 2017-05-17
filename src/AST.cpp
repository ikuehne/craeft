/**
 * @file AST.cpp
 *
 * Implementations for operations on AST nodes.
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

#include <boost/type_index.hpp>

#include "AST.hh"

namespace Craeft {

namespace AST {

/*****************************************************************************
 * Printing the AST for debugging purposes.
 */

/** 
 * @brief Visitor for printing AST::Types.
 */
struct TypePrintVisitor: boost::static_visitor<void> {
    TypePrintVisitor(std::ostream &out): out(out) {}

    void operator()(const Void &_) { out << "Type {Void}"; }

    void operator()(const NamedType &nt) {
        out << "Type {" << nt.name << "}";
    }

    void operator()(const std::unique_ptr<Pointer> &pt) {
        out << "Pointer {";
        boost::apply_visitor(*this, pt->pointed);
        out << "}";
    }

    std::ostream &out;
};

/**
 * @brief Visitor for printing `AST::Expression`s.
 */
struct ExpressionPrintVisitor: boost::static_visitor<void> {
    ExpressionPrintVisitor(std::ostream &out): out(out) {}

    template<typename T>
    void operator()(const T &lit) {
        out << boost::typeindex::type_id<T>().pretty_name()
            << " {" << lit.value << "}";
    }

    void operator()(const Variable &var) {
        out << "Variable {" << var.name << "}";
    }

    void operator()(const std::unique_ptr<Reference> &ref) {
        out << "Reference {";
        boost::apply_visitor(*this, ref->referand);
        out << "}";
    }

    void operator()(const std::unique_ptr<Dereference> &deref) {
        out << "Dereference {";
        boost::apply_visitor(*this, deref->referand);
        out << "}";
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
        out << "Cast {";
        TypePrintVisitor type_printer(out);
        boost::apply_visitor(type_printer, *c->t);
        out << ", ";
        boost::apply_visitor(*this, c->arg);
        out << "}";
    }

    std::ostream &out;
};

struct StatementPrintVisitor: boost::static_visitor<void> {
    StatementPrintVisitor(std::ostream &out): out(out) {}

    void operator()(const std::unique_ptr<AST::Assignment> &assgnt) {
        ExpressionPrintVisitor ev(out);

        out << "Assignment {";
        boost::apply_visitor(ev, assgnt->lhs);
        out << ", ";
        boost::apply_visitor(ev, assgnt->rhs);
        out << "}";
    }

    void operator()(const Expression &expr) {
        ExpressionPrintVisitor ev(out);

        out << "Statement {";

        boost::apply_visitor(ev, expr);

        out << "}";
    }

    void operator()(const std::unique_ptr<Declaration> &decl) {
        TypePrintVisitor tv(out);
        ExpressionPrintVisitor ev(out);

        out << "Declaration {";

        boost::apply_visitor(tv, decl->type);

        out << ", ";

        ev(decl->name);

        out << "}";
    }

    void operator()(const std::unique_ptr<CompoundDeclaration> &decl) {
        TypePrintVisitor tv(out);
        ExpressionPrintVisitor ev(out);

        out << "Declaration {";

        boost::apply_visitor(tv, decl->type);

        out << ", ";

        ev(decl->name);

        out << ", ";

        boost::apply_visitor(ev, decl->rhs);

        out << "}";
    }

    void operator()(const Return &ret) {
        ExpressionPrintVisitor ev(out);

        out << "Return {";

        boost::apply_visitor(ev, *ret.retval);

        out << "}";
    }

    void operator()(const VoidReturn &_) {
        out << "VoidReturn {}";
    }

    void operator()(const std::unique_ptr<IfStatement> &ifstmt) {
        ExpressionPrintVisitor ev(out);

        out << "IfStatement {";

        boost::apply_visitor(ev, ifstmt->condition);

        out << ", ";

        out << "IfTrue {";

        if (ifstmt->if_block.size()) {
            boost::apply_visitor(*this, *ifstmt->if_block.begin());
            for (auto iter = ifstmt->if_block.begin() + 1;
                 iter != ifstmt->if_block.end(); ++iter) {
                out << ", ";
                boost::apply_visitor(*this, *iter);
            }
        }

        out << "}, Else {";

        if (ifstmt->else_block.size()) {
            boost::apply_visitor(*this, *ifstmt->else_block.begin());
            for (auto iter = ifstmt->else_block.begin() + 1;
                 iter != ifstmt->else_block.end(); ++iter) {
                out << ", ";
                boost::apply_visitor(*this, *iter);
            }
        }

        out << "}}";
    }

    std::ostream &out;
};

struct ToplevelPrintVisitor: boost::static_visitor<void> {
    ToplevelPrintVisitor(std::ostream &out): out(out) {}

    void operator()(const TypeDeclaration &tdecl) {
        out << "TypeDeclaration {" << tdecl.name << "}";
    }

    void operator()(const StructDeclaration &sdecl) {
        out << "StructDeclaration {" << sdecl.name;

        StatementPrintVisitor sv(out);

        for (const auto &mem: sdecl.members) {
            out << ", ";
            sv(mem);
        }

        out << "}";
    }

    void operator()(const FunctionDeclaration &fdecl) {
        out << "FunctionDeclaration {" << fdecl.name << ", ";

        StatementPrintVisitor sv(out);

        for (const auto &arg: fdecl.args) {
            sv(arg);
            out << ", ";
        }

        TypePrintVisitor tv(out);

        boost::apply_visitor(tv, fdecl.ret_type);

        out << "}";
    }

    void operator()(const std::unique_ptr<FunctionDefinition> &func) {
        out << "FunctionDefinition {";
        (*this)(func->signature);

        StatementPrintVisitor sv(out);
        for (const auto &arg: func->block) {
            out << ", ";
            boost::apply_visitor(sv, arg);
        }

        out << "}";
    }

    std::ostream &out;
};

void print_expr(const Expression &expr, std::ostream &out) {
    ExpressionPrintVisitor printer(out);
    boost::apply_visitor(printer, expr);
}

void print_statement(const Statement &stmt, std::ostream &out) {
    StatementPrintVisitor printer(out);
    boost::apply_visitor(printer, stmt);
}

void print_toplevel(const TopLevel &top, std::ostream &out) {
    ToplevelPrintVisitor printer(out);
    boost::apply_visitor(printer, top);
}

}

}
