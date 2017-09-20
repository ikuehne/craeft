/**
 * @file ModuleCodegenImpl.cpp
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

#include "StatementCodegen.hh"
#include "ValueCodegen.hh"
#include "TypeCodegen.hh"

namespace Craeft {

void StatementCodegen::operator()(const AST::ExpressionStatement &expr) {
    ValueCodegen vg(_translator);
    vg.visit(expr.expr());
}

void StatementCodegen::operator()(const AST::Assignment &assignment) {
    LValueCodegen lc(_translator);

    auto addr = LValueCodegen(_translator).visit(assignment.lhs());
    auto rhs  = ValueCodegen(_translator).visit(assignment.rhs());

    _translator.add_store(addr, rhs, assignment.pos());
}

void StatementCodegen::operator()(const AST::Return &ret) {
    ValueCodegen vg(_translator);
    _translator.return_(vg.visit(ret.retval()), ret.pos());
}

void StatementCodegen::operator()(const AST::VoidReturn &ret) {
    _translator.return_(ret.pos());
}

void StatementCodegen::operator()(const AST::Declaration &decl) {
    auto t = TypeCodegen(_translator).visit(decl.type());
    _translator.declare(decl.name().name(), t);
}

void StatementCodegen::operator()(const AST::CompoundDeclaration &cdecl) {
    std::string name = cdecl.name().name();
    auto t = TypeCodegen(_translator).visit(cdecl.type());
    Variable result = _translator.declare(name, t);
    _translator.add_store(result.get_val(),
                          ValueCodegen(_translator).visit(cdecl.rhs()),
                          cdecl.pos());
}

void StatementCodegen::operator()(const AST::IfStatement &if_stmt) {
    /* Generate code for the condition. */
    auto cond = ValueCodegen(_translator).visit(if_stmt.condition());

    auto structure = _translator.create_ifthenelse(cond, if_stmt.pos());

    for (const auto &arg: if_stmt.if_block()) {
        visit(*arg);
    }

    _translator.point_to_else(structure);

    // Generate "else" code.
    for (const auto &arg: if_stmt.else_block()) {
        visit(*arg);
    }

    _translator.end_ifthenelse(std::move(structure));
}

}
