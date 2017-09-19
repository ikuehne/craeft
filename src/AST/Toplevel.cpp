/**
 * @file AST/Toplevel.cpp
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

#include "AST/Toplevel.hh"

namespace Craeft {

namespace AST {

namespace {

class ToplevelPrintVisitor: public ToplevelVisitor<void> {
public:
    ToplevelPrintVisitor(std::ostream &out): out(out) {}

private:
    void operator()(const TypeDeclaration &tdecl) override {
        out << "TypeDeclaration {" << tdecl.name() << "}";
    }

    void operator()(const StructDeclaration &sdecl) override {
        out << "StructDeclaration {" << sdecl.name();

        for (const auto &mem: sdecl.members()) {
            out << ", ";
            print_statement(*mem, out);
        }

        out << "}";
    }

    void operator()(const FunctionDeclaration &fdecl) override {
        out << "FunctionDeclaration {" << fdecl.name() << ", ";

        for (const auto &arg: fdecl.args()) {
            print_statement(*arg, out);
            out << ", ";
        }

        print_type(fdecl.ret_type(), out);

        out << "}";
    }

    void operator()(const FunctionDefinition &func) override {
        out << "FunctionDefinition {";
        operator()(func.signature());

        for (const auto &arg: func.block()) {
            out << ", ";
            print_statement(*arg, out);
        }

        out << "}";
    }

    void operator()(const TemplateStructDeclaration &sd) override {
        out << "TemplateStructDeclaration {";

        operator()(sd.decl());

        for (const auto &arg: sd.argnames()) {
            out << ", " << arg;
        }

        out << "}";
    }

    void operator()(const TemplateFunctionDefinition &fd) override {
        out << "TemplateFunctionDefinition {";

        operator()(*fd.def());

        for (const auto &arg: fd.argnames()) {
            out << ", " << arg;
        }

        out << "}";
    }

    std::ostream &out;
};

}

void print_toplevel(const Toplevel &top, std::ostream &out) {
    ToplevelPrintVisitor printer(out);
    printer.visit(top);
}

}

}
