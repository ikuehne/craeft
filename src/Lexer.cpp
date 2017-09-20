/**
 * @file Lexer.cpp
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

#include <cmath>
#include <cctype>
#include <sstream>

#include "Lexer.hh"

namespace {

/**
 * Check if the given char is part of a UTF-8.
 */
inline bool is_unicode(char c) {
    return (uint8_t)c >= 128;
}

}

/**
 * @brief Contains all interfaces internal to the compiler.
 *
 * Should *not* be used for files intended to be compiled to executables.
 */
namespace Craeft {

Lexer::Lexer(const std::string &fname)
    : c(' '),
      eof(false),
      tok(std::make_unique<Tok::OpenParen>()),
      pos(0, 0, std::make_shared<std::string>(fname)),
      stream(fname) {
    shift();
}

SourcePos Lexer::get_pos(void) const {
    return pos;
}

static inline uint8_t digit(char n) {
    /* TODO: add proper error handling. */
    return n - 48;
}

static inline bool is_opchar(char c) {
    return std::string("!:.*=+-><&%^@~/").find(c) != std::string::npos;
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

std::string Lexer::lex_string(void) {
    std::ostringstream result;

    while (true) {
        get();

        if (eof) {
            throw Error("lexer error", "unterminated string", pos);
        }

        if (c == '\\') {
            get();

            switch (c) {
                case 'a':
                    result << '\a';
                    break;
                case 'b':
                    result << '\b';
                    break;
                case 'f':
                    result << '\f';
                    break;
                case 'n':
                    result << '\n';
                    break;
                case 'r':
                    result << '\r';
                    break;
                case 't':
                    result << '\t';
                    break;
                case 'v':
                    result << '\v';
                    break;
                default:
                    result << c;
                    break;
            }

            continue;
        } else if (c == '"') {
            break;
        }

        result << c;
    }

    return result.str();
}

bool Lexer::at_eof(void) const {
    return eof;
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

        while (isalpha(c) || isdigit(c) || c == '_' || is_unicode(c)) {
            tname.push_back(c);
            get();
        }

        tok = std::make_unique<Tok::TypeName>(tname);
    /* Identifiers and identifier-like keywords. */
    } else if (islower(c) || is_unicode(c)) {
        std::string ident;

        while (isalpha(c) || isdigit(c) || c == '_' || is_unicode(c)) {
            ident.push_back(c);
            get();
        }

        /* Keywords that otherwise look like identifiers. */
        if (ident == "fn") {
            tok = std::make_unique<Tok::Fn>();
        } else if (ident == "struct") {
            tok = std::make_unique<Tok::Struct>();
        } else if (ident == "type") {
            tok =  std::make_unique<Tok::Type>();
        } else if (ident == "return") {
            tok =  std::make_unique<Tok::Return>();
        } else if (ident == "if") {
            tok =  std::make_unique<Tok::If>();
        } else if (ident == "else") {
                tok = std::make_unique<Tok::Else>();
        } else if (ident == "while") {
            tok = std::make_unique<Tok::While>();
        /* If none of those, an identifier. */
        } else {
            tok = std::make_unique<Tok::Identifier>(ident);
        }
    /* Numeric literal. */
    } else if (isdigit(c)) {
        auto result = lex_number();
        if (result.which() == 0) {
            auto literal = boost::get<double>(result);
            tok = std::make_unique<Tok::FloatLiteral>(literal);
        } else {
            auto literal = boost::get<uint64_t>(result);
            tok = std::make_unique<Tok::UIntLiteral>(literal);
        }
    /* Operators.  This is easily extensible to user-defined operators. */
    } else if (is_opchar(c)) {
        std::string result;
        result.push_back(c);

        for (get(); is_opchar(c); get()) {
            result.push_back(c);
        }

        tok = std::make_unique<Tok::Operator>(result);
    /* Some random syntax. */
    } else if (c == '(') {
        tok = std::make_unique<Tok::OpenParen>();
        get();
    } else if (c == ')') {
        tok = std::make_unique<Tok::CloseParen>();
        get();
    } else if (c == '{') {
        tok = std::make_unique<Tok::OpenBrace>();
        get();
    } else if (c == '}') {
        tok = std::make_unique<Tok::CloseBrace>();
        get();
    } else if (c == ';') {
        tok = std::make_unique<Tok::Semicolon>();
        get();
    } else if (c == ',') {
        tok = std::make_unique<Tok::Comma>();
        get();
    } else if (c == '"') {
        tok = std::make_unique<Tok::StringLiteral>(lex_string());
        get();
    } else throw Error("lexer error",
                       std::string("character \"") + c + "\" not recognized",
                       pos);
}

const Tok::Token &Lexer::get_tok(void) const {
    return *tok;
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
