/**
 * @file Lexer.hh
 *
 * @brief Lexes a character stream into a `Token` stream.
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

#include <iostream>
#include <string>
#include <fstream>
#include <memory>

#include "Error.hh"
#include "Token.hh"

namespace Craeft {

class Lexer {
public:
    /**
     * @brief Create a new lexer, tokenizing the given input stream.
     *
     * @param fname The name of the file to tokenize.
     */
    Lexer(const std::string &fname);

    /**
     * @brief Get the position the lexer is currently at.
     */
    SourcePos get_pos(void) const;

    /**
     * @brief Return the last lexed token.
     */
    const Tok::Token &get_tok(void) const;

    /**
     * @brief Return whether the lexer has reached the end of the stream.
     */
    bool at_eof() const;

    /**
     * @brief Lex a new token.
     */
    void shift(void);

private:
    char c;
    void get(void);
    boost::variant<double, uint64_t> lex_number(void);
    std::string lex_string(void);

    bool eof;
    std::unique_ptr<Tok::Token> tok;
    SourcePos pos;
    std::ifstream stream;
};

}
