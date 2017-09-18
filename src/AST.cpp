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
class TypePrintVisitor: public TypeVisitor<void> {
public:
    TypePrintVisitor(std::ostream &out): out(out) {}

    ~TypePrintVisitor(void) override {}

private:
    void operator()(const NamedType &nt) override {
        out << "Type {" << nt.name() << "}";
    }

    void operator()(const Void &_) override { out << "Type {Void}"; }

    void operator()(const TemplatedType &t) override {
        out << "TemplatedType {" << t.name();

        for (const auto &arg: t.args()) {
            visit(*arg);
            out << ", ";
        }

        out << "}";
    }

    void operator()(const Pointer &pt) override {
        out << "Pointer {";
        visit(pt.pointed());
        out << "}";
    }

    std::ostream &out;
};

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

        TypePrintVisitor tv(out);

        if (fc.type_args().size()) {
            tv.visit(*fc.type_args()[0]);
        }

        for (unsigned i = 1; i < fc.type_args().size(); ++i) {
            out << ", ";
            tv.visit(*fc.type_args()[i]);
        }

        for (const auto &arg: fc.value_args()) {
            out << ", ";
            visit(*arg);
        }

        out << "}";
    }

    void operator()(const Cast &c) override {
        out << "Cast {";
        TypePrintVisitor type_printer(out);
        type_printer.visit(c.type());
        out << ", ";
        visit(c.arg());
        out << "}";
    }

    std::ostream &out;
};

struct StatementPrintVisitor: boost::static_visitor<void> {
    StatementPrintVisitor(std::ostream &out): out(out) {}

    void operator()(const std::unique_ptr<AST::Assignment> &assgnt) {
        ExpressionPrintVisitor ev(out);

        out << "Assignment {";
        ExpressionPrintVisitor(out).visit(*assgnt->lhs);
        out << ", ";
        ev.visit(*assgnt->rhs);
        out << "}";
    }

    void operator()(const std::unique_ptr<Expression> &expr) {
        ExpressionPrintVisitor ev(out);

        out << "Statement {";

        ev.visit(*expr);

        out << "}";
    }

    void operator()(const std::unique_ptr<Declaration> &decl) {
        TypePrintVisitor tv(out);
        ExpressionPrintVisitor ev(out);

        out << "Declaration {";

        tv.visit(*decl->type);

        out << ", ";

        ev.visit(decl->name);

        out << "}";
    }

    void operator()(const std::unique_ptr<CompoundDeclaration> &decl) {
        TypePrintVisitor tv(out);
        ExpressionPrintVisitor ev(out);

        out << "Declaration {";

        tv.visit(*decl->type);

        out << ", ";

        ev.visit(decl->name);

        out << ", ";

        ev.visit(*decl->rhs);

        out << "}";
    }

    void operator()(const Return &ret) {
        ExpressionPrintVisitor ev(out);

        out << "Return {";

        ev.visit(*ret.retval);

        out << "}";
    }

    void operator()(const VoidReturn &_) {
        out << "VoidReturn {}";
    }

    void operator()(const std::unique_ptr<IfStatement> &ifstmt) {
        ExpressionPrintVisitor ev(out);

        out << "IfStatement {";

        ev.visit(*ifstmt->condition);

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

        tv.visit(*fdecl.ret_type);

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

    void operator()(const std::shared_ptr<FunctionDefinition> &func) {
        out << "FunctionDefinition {";
        (*this)(func->signature);

        StatementPrintVisitor sv(out);
        for (const auto &arg: func->block) {
            out << ", ";
            boost::apply_visitor(sv, arg);
        }

        out << "}";
    }

    void operator()(const TemplateStructDeclaration &sd) {
        out << "TemplateStructDeclaration {";

        (*this)(sd.decl);

        for (const auto &arg: sd.argnames) {
            out << ", " << arg;
        }

        out << "}";
    }

    void operator()(const TemplateFunctionDefinition &fd) {
        out << "TemplateFunctionDefinition {";

        (*this)(fd.def);

        for (const auto &arg: fd.argnames) {
            out << ", " << arg;
        }

        out << "}";
    }

    std::ostream &out;
};

void print_expr(const Expression &expr, std::ostream &out) {
    ExpressionPrintVisitor printer(out);
    printer.visit(expr);
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
