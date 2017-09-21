/**
 * @file AST/AST.hh
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

#include "Error.hh"

namespace Craeft {

namespace AST {

/**
 * @brief Nodes in the abstract syntax tree.
 */
class ASTNode {
public:
    explicit ASTNode(SourcePos pos): _pos(pos) {}
    virtual ~ASTNode() {}

    /**
     * The only thing all AST nodes have in common is a location in the
     * source.
     */
    SourcePos pos(void) const { return _pos; }
    void set_pos(SourcePos pos) { _pos = pos; }
private:
    SourcePos _pos;
};

}
}
