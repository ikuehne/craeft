/**
 * @file Error.hh
 *
 * @brief Error handling and related utilities.
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

#include <cstdint>
#include <string>

namespace Craeft {

/**
 * @brief Represent a position in a source file.
 *
 * Must be small (32 bits) because it is used to annotate all AST nodes, and
 * they get passed around a lot.  Intended to allow for pretty and informative
 * error messages.
 */
struct SourcePos {
    uint16_t charno;
    uint16_t lineno;

    SourcePos(uint16_t charno, uint16_t lineno): charno(charno),
                                                 lineno(lineno) {}
};

/**
 * @brief A basic Craeft error, containing a message and a source position.
 */
class Error {
public:
    /**
     * @brief Create a new Error.
     *
     * @param header A header which describes the type of error, e.g. "Parser
     *               error".
     * @param message The message to print.  Should briefly explain the error,
     *                and should not be a complete sentence.
     * @param filename The file in which the error occured.
     * @param pos The position in the source where the error occured.
     */
    Error(std::string header, std::string message, std::string filename, SourcePos pos);

    /**
     * @brief Print the error to the given stream.
     */
    void emit(std::ostream &);
private:
    std::string header;
    std::string msg;
    std::string fname;
    SourcePos pos;
};

}
