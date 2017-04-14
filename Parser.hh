/**
 * @file Parser.hh
 *
 * @brief The parser.
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

#include <map>

#include "Expression.hh"
#include "Lexer.hh"

namespace Craeft {

class Parser {
public:
    /**
     * @brief Create a new Parser, parsing from the given file.
     *
     * @param fname The filename to open and parse from.
     */
    Parser(std::string fname);

    /**
     * @brief Parse the next expression from the lexer.
     *
     * Start at the token the lexer is *currently* on.
     */
    AST::Expression parse_expression(void);

private:
    /**
     * @brief Parse a variable or a function call.
     */
    AST::Expression parse_variable(void);

    /**
     * @brief Parse a series of binops, given the first one.
     */
    AST::Expression parse_binop(int prec, AST::Expression lhs);

    /**
     * @brief Parse a cast.
     *
     * Current token should be at the typename, e.g.
     *
     * (Double)5
     *  ^
     */
    std::unique_ptr<AST::Cast> parse_cast(void);

    /**
     * @brief Parse a parenthesized expression.
     */
    AST::Expression parse_parens(void);

    /**
     * @brief Parse anything but an operator application.
     */
    AST::Expression parse_primary(void);

    /**
     * @brief Look up the precedence of an operator.
     */
    int get_token_precedence(void) const;

    /**
     * @brief The held lexer.
     */
    Lexer lexer;

    /**
     * @brief The map of operator precedences.
     */
    std::map<std::string, int> precedences {
        {"=", 2},
        {"<", 10},
        {"+", 20},
        {"-", 20},
        {"*", 40},
        {"/", 40},
    };
};

}
