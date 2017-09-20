/**
 * @file Codegen/Statement.cpp
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

#include "Codegen/Statement.hh"
#include "Codegen/Value.hh"
#include "Codegen/Type.hh"

namespace Craeft {

namespace Codegen {

void StatementGen::operator()(const AST::ExpressionStatement &expr) {
    ValueGen vg(_translator);
    vg.visit(expr.expr());
}

void StatementGen::operator()(const AST::Assignment &assignment) {
    LValueGen lc(_translator);

    auto addr = LValueGen(_translator).visit(assignment.lhs());
    auto rhs  = ValueGen(_translator).visit(assignment.rhs());

    _translator.add_store(addr, rhs, assignment.pos());
}

void StatementGen::operator()(const AST::Return &ret) {
    ValueGen vg(_translator);
    _translator.return_(vg.visit(ret.retval()), ret.pos());
}

void StatementGen::operator()(const AST::VoidReturn &ret) {
    _translator.return_(ret.pos());
}

void StatementGen::operator()(const AST::Declaration &decl) {
    auto t = TypeGen(_translator).visit(decl.type());
    _translator.declare(decl.name().name(), t);
}

void StatementGen::operator()(const AST::CompoundDeclaration &cdecl) {
    std::string name = cdecl.name().name();
    auto t = TypeGen(_translator).visit(cdecl.type());
    Variable result = _translator.declare(name, t);
    _translator.add_store(result.get_val(),
                          ValueGen(_translator).visit(cdecl.rhs()),
                          cdecl.pos());
}

void StatementGen::operator()(const AST::IfStatement &if_stmt) {
    /* Generate code for the condition. */
    auto cond = ValueGen(_translator).visit(if_stmt.condition());

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
}
