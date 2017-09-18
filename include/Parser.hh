/**
 * @file Parser.hh
 *
 * @brief The interface to the parser.
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

#include <memory>
#include <string>

#include "AST.hh"

namespace Craeft {

class ParserImpl;

class Parser {
public:
    /**
     * @brief Create a new Parser, parsing from the given file.
     *
     * @param fname The filename to open and parse from.
     */
    Parser(std::string fname);

    /* Explicitly declared because PImpl. */
    ~Parser();

    /**
     * @brief Parse the next expression from the stream.
     */
    std::unique_ptr<AST::Expression> parse_expression(void);

    /**
     * @brief Parse the next statement from the stream.
     */
    AST::Statement parse_statement(void);

    /**
     * @brief Parse the next top-level AST node from the stream.
     */
    AST::TopLevel parse_toplevel(void);

    /**
     * @brief Return whether the parser has reached the end of the stream.
     */
    bool at_eof(void);

private:
    std::unique_ptr<ParserImpl> pimpl;
};

}
