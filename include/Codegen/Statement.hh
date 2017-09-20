/**
 * @file Codegen/Statement.hh
 *
 * @brief Codegen for AST statements.
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

#include "AST/Statements.hh"
#include "Translator.hh"

namespace Craeft {

namespace Codegen {

/**
 * Codegen for statements: pass them on to the Translator.
 */
class StatementGen: public AST::StatementVisitor<void> {
public:
    StatementGen(Translator &translator): _translator(translator) {}

private:
    // Visitors of different AST statement types.
    void operator()(const AST::ExpressionStatement &);
    void operator()(const AST::VoidReturn &);
    void operator()(const AST::Return &);
    void operator()(const AST::Assignment &assignment);
    void operator()(const AST::Declaration &);
    void operator()(const AST::CompoundDeclaration &);
    void operator()(const AST::IfStatement &);

    Translator &_translator;
};

}
}
