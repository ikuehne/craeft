/**
 * @file ParserImpl.hh
 *
 * @brief The actual implementation of the parser.
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

#include "AST.hh"
#include "Lexer.hh"

namespace Craeft {

/**
 * @brief Actual implementation of Parser functionality.
 *
 * See Parser.hh for documentation of public methods.
 */
class ParserImpl {
public:
    ParserImpl(std::string fname);

    /**
     * @brief Parse the next expression from the lexer.
     *
     * Start at the token the lexer is *currently* on.
     */
    AST::Expression parse_expression(void);

    /**
     * Note: starts at the token the lexer is *currently* on.
     */
    AST::Statement parse_statement(void);
    AST::TopLevel parse_toplevel(void);
    bool at_eof(void) const;

    /*************************************************************************
     * AST-handling utilities.
     */
    inline void verify_expression(const AST::Expression &) const;
    inline AST::LValue to_lvalue(AST::Expression, SourcePos pos) const;
    inline AST::Statement extract_assignments(AST::Expression &) const;

private:
    /**
     * @brief Parse a variable or a function call.
     */
    AST::Expression parse_variable(void);

    /**
     * @brief Parse a unary operator invocation.
     */
    AST::Expression parse_unary(void);

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
     * @brief Parse a type.
     */
    AST::Type parse_type(void);

    /**
     * @brief Parse a variable declaration.
     */
    AST::Statement parse_declaration(void);

    /**
     * @brief Parse an if statement.
     */
    std::unique_ptr<AST::IfStatement> parse_if_statement(void);

    /**
     * @brief Parse a return statement.
     */
    AST::Statement parse_return(void);
    
    AST::TypeDeclaration parse_type_declaration(void);

    AST::StructDeclaration parse_struct_declaration(void);

    AST::TopLevel parse_function(void);

    /**
     * @brief Look up the precedence of an operator.
     */
    int get_token_precedence(void) const;

    /*************************************************************************
     * Error-handling utilities.
     */

    template<typename T>
    inline void find_and_shift(std::string at_place);

    [[noreturn]] inline void _throw(std::string message);

    /**
     * @brief The held lexer.
     */
    Lexer lexer;

    /**
     * @brief The map of operator precedences.
     */
    std::map<std::string, int> precedences {
        {"=", 200},
        {"||", 300},
        {"&&", 400},
        {"|", 500},
        {"^", 600},
        {"&", 700},
        {"==", 800},
        {"!=", 800},

        {"<", 900},
        {"<=", 900},
        {">", 900},
        {">=", 900},

        {"<<", 1000},
        {">>", 1000},

        {"+", 1100},
        {"-", 1100},

        {"*", 1200},
        {"/", 1200},
        {"%", 1200},

        {".", 1400},
        {"->", 1400},
    };

    /**
     * @brief The name of the file being parsed.
     */
    std::string fname;
};

}
