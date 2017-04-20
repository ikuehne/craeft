/**
 * @file Parser.cpp
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

#include <cstdlib>

#include "Parser.hh"
#include "VariantUtils.hh"

/*****************************************************************************
 * Utilities for converting tokens to simple AST nodes.
 */

namespace Craeft {

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

AST::Statement Parser::parse_statement(void) {
    AST::Statement result;
    if (is_type<Tok::TypeName>(lexer.get_tok())) {
        result = parse_declaration();
    } else {
        result = std::make_unique<AST::Expression>(parse_expression());
    }

    if (!is_type<Tok::Semicolon>(lexer.get_tok())) {
        throw "Expected ; after statement.";
    }

    // Shif the semicolon.
    lexer.shift();

    return result;
}

AST::TopLevel Parser::parse_toplevel(void) {
    if (is_type<Tok::Fn>(lexer.get_tok())) {
        return parse_function();
    } else if (is_type<Tok::Struct>(lexer.get_tok())) {
        return parse_struct_declaration();
    } else if (is_type<Tok::Type>(lexer.get_tok())) {
        return parse_type_declaration();
    } else {
        throw "Expected function or type declaration at top level.";
    }
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
    auto type = std::make_unique<AST::Type>(parse_type());

    // No closing parenthesis.
    if (!is_type<Tok::CloseParen>(lexer.get_tok())) {
        throw "Expected close paren after type in cast.";
    }

    // Shift the closing paren.
    lexer.shift();

    auto expr = parse_expression();

    return std::make_unique<AST::Cast>(std::move(type), std::move(expr),
                                       start);
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

AST::Type Parser::parse_type(void) {
    /* TODO: Handle pointers, parentheses, array types, etc. */
    auto tname = boost::get<Tok::TypeName>(lexer.get_tok()).name;

    // Shift off the typename.
    lexer.shift();

    if (tname == "Double") {
        return AST::Double();
    } else if (tname == "Float") {
        return AST::Float();
    } else if (tname.size() == 3 && isdigit(tname[1]) && isdigit(tname[2])) {
        int nbits = std::stoi(tname.substr(1, 2));
        if (nbits <= 64) {
            if      (tname[0] == 'I') return AST::IntType(nbits);
            else if (tname[0] == 'U') return AST::UIntType(nbits);
        }
    }

    return AST::UserType(tname);
}

AST::Statement Parser::parse_declaration(void) {
    if (!is_type<Tok::TypeName>(lexer.get_tok())) {
        throw "Expected type name in declaration.";
    }

    auto type = parse_type();

    if (!is_type<Tok::Identifier>(lexer.get_tok())) {
        throw "Expected identifier in declaration.";
    }

    auto ident = boost::get<Tok::Identifier>(lexer.get_tok());

    auto var = AST::Variable(ident.name, lexer.get_pos());

    lexer.shift();

    if (is_type<Tok::Semicolon>(lexer.get_tok())) {
        return std::make_unique<AST::Declaration>(std::move(type), var);
    }

    if (!is_type<Tok::Operator>(lexer.get_tok())) {
        throw "Expected equals sign in compound assignment.";
    }

    auto op = boost::get<Tok::Operator>(lexer.get_tok());

    if (op.op != "=") {
        throw "Expected equals sign in compound assignment.";
    }

    // Shift the equals sign.
    lexer.shift();

    auto rhs = parse_expression();

    return std::make_unique<AST::CompoundDeclaration>(std::move(type), var,
                                                      std::move(rhs));
}

AST::TypeDeclaration Parser::parse_type_declaration(void) {
    // Shift the `type`.
    lexer.shift();

    auto tok = lexer.get_tok();
    auto *tname = boost::get<Tok::TypeName>(&tok);

    if (!tname) {
        throw "Expected type name in type declaration.";
    }

    // Shift the type name.
    lexer.shift();

    return AST::TypeDeclaration(tname->name);
}

AST::StructDeclaration Parser::parse_struct_declaration(void) {
    // Shift the `struct`.
    lexer.shift();

    auto tok = lexer.get_tok();
    auto *tname = boost::get<Tok::TypeName>(&tok);

    if (!tname) {
        throw "Expected type name in type declaration.";
    }

    // Shift the type name.
    lexer.shift();

    if (!is_type<Tok::OpenBrace>(lexer.get_tok())) {
        throw "Expected open brace in struct definition.";
    }

    // Shift the brace.
    lexer.shift();

    std::vector<std::unique_ptr<AST::Declaration> > members;

    // Until we get to the closing brace,
    while (!is_type<Tok::CloseBrace>(lexer.get_tok())) {
        auto decl_tmp = parse_declaration();
        auto *decl = boost::get<std::unique_ptr<AST::Declaration>>(&decl_tmp);

        if (!decl) {
            throw "Expected variable declaration in struct definition.";
        }

        if (!is_type<Tok::Semicolon>(lexer.get_tok())) {
            throw "Expected semicolon after struct member declaration.";
        }

        members.push_back(std::move(*decl));

        // Shift the semicolon.
        lexer.shift();
    }

    // Shift the closing brace.
    lexer.shift();

    return AST::StructDeclaration(tname->name, std::move(members));
}

AST::TopLevel Parser::parse_function(void) {
    // Shift the `fn`.
    lexer.shift();

    auto tok = lexer.get_tok();

    auto *ident = boost::get<Tok::Identifier>(&tok);

    if (!ident) {
        throw "Expected identifier as function name.";
    }

    auto fname = std::move(ident->name);

    // Shift the function name.
    lexer.shift();

    if (!is_type<Tok::OpenParen>(lexer.get_tok())) {
        throw "Expected open parenthesis to start argument list.";
    }

    // Shift the open paren.
    lexer.shift();

    std::vector<std::unique_ptr<AST::Declaration> > args;

    while (!is_type<Tok::CloseParen>(lexer.get_tok())) {
        auto decl_tmp = parse_declaration();
        auto *decl = boost::get<std::unique_ptr<AST::Declaration>>(&decl_tmp);

        if (!decl) {
            throw "Expected variable declaration in argument list.";
        }

        args.push_back(std::move(*decl));

        if      (is_type<Tok::CloseParen>(lexer.get_tok())) break;
        else if (is_type<Tok::Comma>(lexer.get_tok())) continue;
        else    throw "Expected comma between arguments in declaration.";
    }

    // Shift the closing paren.
    lexer.shift();

    // Default to void type;
    AST::Type ret_type = AST::Void();

    // parse another return type if present.
    if (is_type<Tok::Arrow>(lexer.get_tok())) {
        lexer.shift();
        ret_type = parse_type();
    }

    AST::FunctionDeclaration decl(fname,
                                  std::move(args), std::move(ret_type));

    // If semicolon, this is just a forward declaration.
    if (is_type<Tok::Semicolon>(lexer.get_tok())) {
        return std::move(decl);
    }

    if (!is_type<Tok::OpenBrace>(lexer.get_tok())) {
        throw "Expected open brace before function body.";
    }

    // Shift off open brace.
    lexer.shift();

    std::vector<AST::Statement> body;

    while (!is_type<Tok::CloseBrace>(lexer.get_tok())) {
        body.push_back(parse_statement());

        if (!is_type<Tok::Semicolon>(lexer.get_tok())) {
            throw "Expected semicolon after statement.";
        }

        // Shift off the semicolon.
        lexer.shift();
    }

    return std::make_unique<AST::FunctionDefinition>(std::move(decl),
                                                     std::move(body));
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
