/**
 * @file Token.hh
 *
 * @brief Tokens as output by the lexer.
 *
 * Consists of a series of small classes grouped in the namespace `Tok`, and a
 * variant type `Token` which can be any of those classes.
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

#include <boost/variant.hpp>

namespace Craeft {

/**
 * @brief Contains all classes and utilities relating to lexical tokens.
 */
namespace Tok {

/**
 * @defgroup Tokens Classes for possible types of tokens.
 *
 * Each of these classes contains public data members for the contents of the
 * token and a simple inline constructor which takes one argument per member.
 * For performance, these constructors will `move` potentially
 * expensive-to-copy data like strings.
 *
 * @{
 */

/**
 * @brief Names of types.
 */
struct TypeName {
    std::string name;

    TypeName(std::string name): name(std::move(name)) {}
};

/**
 * @brief Non-type identifiers.
 */
struct Identifier {
    std::string name;

    Identifier(std::string name): name(std::move(name)) {}
};

/**
 * @brief Signed integer literals.
 */
struct IntLiteral {
    int64_t value;

    IntLiteral(int64_t value): value(value) {}
};

/**
 * @brief Unsigned integer literals.
 */
struct UIntLiteral {
    uint64_t value;

    UIntLiteral(uint64_t value): value(value) {}
};

/**
 * @brief Floating-point literals.
 */
struct FloatLiteral {
    double value;

    FloatLiteral(double value): value(value) {}
};

/**
 * @brief Operators.
 */
struct Operator {
    std::string op;

    Operator(std::string op): op(std::move(op)) {}
};

/**
 * @defgroup Empty Tokens with no meaning other than disambiguating syntax.
 */

struct OpenParen {
    static inline std::string repr(void) { return "("; }
};
struct CloseParen {
    static inline std::string repr(void) { return ")"; }
};
struct OpenBrace {
    static inline std::string repr(void) { return "{"; }
};
struct CloseBrace {
    static inline std::string repr(void) { return "}"; }
};
struct Comma {
    static inline std::string repr(void) { return ","; }
};
struct Semicolon {
    static inline std::string repr(void) { return ";"; }
};
struct Fn {
    static inline std::string repr(void) { return "fn"; }
};
struct Struct {
    static inline std::string repr(void) { return "struct"; }
};
struct Type {
    static inline std::string repr(void) { return "type"; }
};
struct Return {
    static inline std::string repr(void) { return "return"; }
};
struct If {
    static inline std::string repr(void) { return "if"; }
};
struct Else {
    static inline std::string repr(void) { return "else"; }
};
struct While {
    static inline std::string repr(void) { return "while"; }
};

/** @} */

/** The actual `Token` class: a discriminated union of token types. */
typedef boost::variant< TypeName,
                        Identifier,
                        IntLiteral, UIntLiteral, FloatLiteral,
                        Operator,
                        OpenParen, CloseParen,
                        OpenBrace, CloseBrace,
                        Comma, Semicolon,
                        Fn, Struct, Type,
                        Return,
                        If, Else, While > Token;

}

}
