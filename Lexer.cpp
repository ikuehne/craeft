/**
 * @file Lexer.cpp
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

#include <cmath>
#include <cctype>

#include "Lexer.hh"

namespace Compiler {

Lexer::Lexer(std::string fname)
    : c(' '), eof(false), tok(Tok::OpenParen()), pos(0, 0), stream(fname) {
    shift();
}

SourcePos Lexer::get_pos(void) const {
    return pos;
}

static inline uint8_t digit(char n) {
    switch (n) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        default:  return 0; /* TODO: add proper error handling. */
    }
}

static inline bool is_opchar(char c) {
    return std::string("*=+-><&%^@~/").find(c) != std::string::npos;
}

boost::variant<double, uint64_t> Lexer::lex_number(void) {
    uint64_t num = 0;

    /* Parse the initial series of digits. */
    while (isdigit(c)) {
        num *= 10;
        num += digit(c);
        get();
    }

    /* If a decimal, */
    if (c == '.') {
        /* Parse the decimal bit. */
        double result = (double)num;
        double decimal_places = 0.1;
        for (get(); isdigit(c); get()) {
            result += decimal_places * digit(c);
            decimal_places *= 0.1;
        }


        /* Parse the exponent if present. */
        if (c == 'e' || c == 'E') {
            get();

            bool neg = false;

            if (c == '-') {
                get();
                neg = true;
            }

            uint64_t exp = 0;
            for (; isdigit(c); get()) {
                exp *= 10;
                exp += digit(c);
            }

            return result * pow(10.0, (neg? -1: 1) * exp);
        }

        return result;
    } else if (c == 'e' || c == 'E') {
        get();

        bool neg = false;
        if (c == '-') {
            get();
            neg = true;
        }

        double result = (double)num;

        double exp = 0;
        for (; isdigit(c); get()) {
            exp *= 10.0;
            exp += (double)digit(c);
        }

        return result * pow(10.0, (neg? -1: 1) * exp);
    } else {
        return num;
    }
}

void Lexer::shift(void) {
    while (std::isspace(c)) {
        get();
    }

    if (c == std::char_traits<char>::eof()) {
        eof = true;
        return;
    }

    /* Type name. */
    if (isupper(c)) {
        std::string tname;

        while (isalpha(c)) {
            tname.push_back(c);
            get();
        }

        tok = Tok::TypeName(tname);
    /* Identifiers and identifier-like keywords. */
    } else if (islower(c)) {
        std::string ident;

        while (isalpha(c)) {
            ident.push_back(c);
            get();
        }

        /* Keywords that otherwise look like identifiers. */
        if      (ident == "fn")     tok =  Tok::Fn();
        else if (ident == "struct") tok =  Tok::Struct();
        else if (ident == "return") tok =  Tok::Return();
        else if (ident == "if")     tok =  Tok::If();
        else if (ident == "else")   tok =  Tok::Else();
        else if (ident == "while")  tok =  Tok::While();
        /* If none  of those, an identifier. */
        else                         tok = Tok::Identifier(ident);
    /* Numeric literal. */
    } else if (isdigit(c)) {
        auto result = lex_number();
        if (result.which() == 0) {
            tok = Tok::FloatLiteral(boost::get<double>(result));
        } else {
            tok = Tok::UIntLiteral(boost::get<uint64_t>(result));
        }
    /* Operators.  This is easily extensible to user-defined operators. */
    } else if (is_opchar(c)) {
        std::string result;
        result.push_back(c);

        for (get(); is_opchar(c); get()) {
            result.push_back(c);
        }

        tok = Tok::Operator(result);
    /* Some random syntax. */
    } else if (c == '(') {
        tok = Tok::OpenParen();
        get();
    } else if (c == ')') {
        tok = Tok::CloseParen();
        get();
    } else if (c == '{') {
        tok = Tok::OpenBrace();
        get();
    } else if (c == '}') {
        tok = Tok::CloseBrace();
        get();
    } else throw "Character not recognized";
}

Tok::Token Lexer::get_tok(void) const {
    return tok;
}

void Lexer::get(void) {
    c = stream.get();

    if (c == '\n' || c == '\r') {
        pos.lineno++;
        pos.charno = 0;
    } else {
        pos.charno++;
    }
}

}
