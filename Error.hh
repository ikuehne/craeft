/**
 * @file Error.hh
 *
 * @brief Error handling and related utilities.
 */

/* CS 81 Compiler: a compiler for a new systems programming language.
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

#include <cstdint>

namespace Compiler {

struct SourcePos {
    uint16_t charno;
    uint16_t lineno;

    SourcePos(uint16_t charno, uint16_t lineno): charno(charno),
                                                 lineno(lineno) {}
};

}
