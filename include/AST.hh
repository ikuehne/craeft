/**
 * @file AST.hh
 *
 * @brief The classes comprising the abstract syntax tree.
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

#include <ostream>
#include <vector>

#include <boost/variant.hpp>

#include "AST/Expressions.hh"
#include "AST/Types.hh"
#include "AST/Statements.hh"
#include "AST/Toplevel.hh"
#include "Error.hh"

namespace Craeft {

/**
 * @brief Contains all classes and utilities relating to the abstract syntax
 * tree.
 */
namespace AST {

}

}
