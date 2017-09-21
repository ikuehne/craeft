/**
 * @file AST/Types.cpp
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

#include "AST/Types.hh"

namespace Craeft {

namespace AST {

namespace {

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

}

void print_type(const Type &type, std::ostream &out) {
    TypePrintVisitor printer(out);
    printer.visit(type);
}

}

}
