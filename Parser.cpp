/**
 * @file Parser.cpp
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

#include "Parser.hh"

#include "VariantUtils.hh"

/*****************************************************************************
 * Utilities for converting tokens to simple AST nodes.
 */

namespace Compiler {

template <typename AstLiteral, typename TokLiteral>
static inline AstLiteral get_literal(TokLiteral tok, SourcePos pos) {
    return AstLiteral(tok.value, pos);
}

/*****************************************************************************
 * Parser public methods.
 */

Parser::Parser(std::string fname): lexer(fname) {}

AST::Expression Parser::parse_expression(void) {
    return parse_binop(0, parse_primary());
}

/*****************************************************************************
 * Parser methods for dealing with particular forms.
 */

AST::Expression Parser::parse_variable(void) {
    auto tok = boost::get<Tok::Identifier>(lexer.get_tok());

    std::string id = tok.name;

    // Shift the name.
    lexer.shift();

    /* Case not function call. */
    if (!is_type<Tok::OpenParen>(lexer.get_tok())) {
        return AST::Variable(id, lexer.get_pos());
    }

    // Shift the opening paren.
    lexer.shift();

    // Accumulate vector of args.
    std::vector<AST::Expression> args;

    if (!is_type<Tok::CloseParen>(lexer.get_tok())) {
        while (true) {
            AST::Expression arg(parse_expression());
            args.push_back(std::move(arg)); 

            /* If close paren, we've reached the end of the list. */
            if (is_type<Tok::CloseParen>(lexer.get_tok())) {
                break;
            }

            // Expect comma before next arg.
            if (!is_type<Tok::Comma>(lexer.get_tok())) {
                throw ("Expected comma or close paren in "
                       "function argument list.");
            }

            // Shift the comma.
            lexer.shift();
        }
    }

    // Shift the close paren.
    lexer.shift();

    return std::make_unique<AST::FunctionCall>(
            std::move(id), std::move(args), lexer.get_pos());
}

AST::Expression Parser::parse_binop(int prec, AST::Expression lhs) {
    auto start = lexer.get_pos();

    while (true) {
        int old_prec = get_token_precedence();

        std::cerr << old_prec << ", " << prec << std::endl;

        if (old_prec < prec) return lhs;

        // Thing in expression was not an operator.
        if (!is_type<Tok::Operator>(lexer.get_tok())) {
            assert(lexer.get_tok().which() != 5);
            throw "Expected operator in arithmetic expression";
        }

        auto op = boost::get<Tok::Operator>(lexer.get_tok());

        lexer.shift();

        auto rhs = parse_primary();
        
        int new_prec = get_token_precedence();

        if (old_prec < new_prec) {
            rhs = parse_binop(old_prec + 1, std::move(rhs));
        }

        lhs = std::make_unique<AST::Binop>(
                op.op, std::move(lhs), std::move(rhs), start);
    }
}

std::unique_ptr<AST::Cast> Parser::parse_cast(void) {

    auto start = lexer.get_pos();
    auto tname = boost::get<Tok::TypeName>(lexer.get_tok());

    // Shift the typename.
    lexer.shift();

    // No closing parenthesis.
    if (!is_type<Tok::CloseParen>(lexer.get_tok())) {
        throw "Expected close paren after type in cast.";
    }

    // Shift the closing paren.
    lexer.shift();

    auto expr = parse_expression();

    auto t = std::make_unique<Type>(std::move(tname.name));

    return std::make_unique<AST::Cast>(std::move(t), std::move(expr), start);
}

AST::Expression Parser::parse_parens(void) {
    auto start = lexer.get_pos();

    // Shift the opening paren.
    lexer.shift();

    // Might be a cast.
    if (is_type<Tok::TypeName>(lexer.get_tok())) {
        auto cast = parse_cast();

        // Fix starting position of cast to opening paren.
        cast->pos = start;

        return std::move(cast);
    }

    auto contents = parse_expression();

    // No closing paren.
    if (!is_type<Tok::CloseParen>(lexer.get_tok())) {
        throw "Expected close paren in parenthesized expression";
    }

    // Shift closing paren.
    lexer.shift();

    return contents;
}

AST::Expression Parser::parse_primary(void) {
    auto tok = lexer.get_tok();
    if        (is_type<Tok::Identifier>(tok)) {
        return parse_variable();
    } else if (auto *i_lit = boost::get<Tok::IntLiteral>(&tok)) {
        auto result = get_literal<AST::IntLiteral>
                                 (*i_lit, lexer.get_pos());
        lexer.shift();
        return result;
    } else if (auto *u_lit = boost::get<Tok::UIntLiteral>(&tok)) {
        auto result = get_literal<AST::UIntLiteral>
                                 (*u_lit, lexer.get_pos());
        lexer.shift();
        return result;
    } else if (auto *f_lit = boost::get<Tok::FloatLiteral>(&tok)) {
        auto result = get_literal<AST::FloatLiteral>
                                 (*f_lit, lexer.get_pos());
        lexer.shift();
        return result;
    } else if (is_type<Tok::OpenParen>(tok)) {
        return parse_parens();
    } else {
        throw "Expected expression.";
    }
}

int Parser::get_token_precedence(void) const {
    auto tok = lexer.get_tok();

    if (auto *op = boost::get<Tok::Operator>(&tok)) {
        auto i = precedences.find(op->op);
        if (i != precedences.end()) {
            return i->second;
        }
    }

    return -1;
}

}
