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

#include "llvm/Support/Casting.h"

#include <boost/variant.hpp>

namespace Craeft {

/**
 * @brief Contains all classes and utilities relating to lexical tokens.
 */
namespace Tok {

/**
 * @brief Craeft lexemes.
 *
 * Uses LLVM RTTI, which is rather heavy-duty for a token stream; should
 * perhaps refactor.
 */
class Token {
public:
    enum TokenKind {
        TypeName,
        Identifier,
        IntLiteral,
        UIntLiteral,
        FloatLiteral,
        StringLiteral,
        Operator,
        OpenParen,
        CloseParen,
        OpenBrace,
        CloseBrace,
        Comma,
        Semicolon,
        Fn,
        Struct,
        Type,
        Return,
        If,
        Else,
        While,
        InvalidToken
    };

    Token(TokenKind kind): _kind(kind) {}

    TokenKind kind(void) const {
        return _kind;
    }

    virtual bool operator==(const Token& other) const = 0;

    bool operator!=(const Token& other) const {
        return !(operator==(other));
    }

    virtual ~Token() {}

    virtual std::string repr(void) const = 0;

private:
    TokenKind _kind;
};

/**
 * Just expands to some boilerplate for LLVM RTTI.
 */
#define TOK_CLASS(X)\
    static bool classof(const Token *t) {\
        return t->kind() == TokenKind::X;\
    }\
    virtual bool operator==(const Token&) const override;\
    ~X(void) override {}

/**
 * @brief Names of types.
 */
struct TypeName: public Token {
    std::string name;

    virtual std::string repr(void) const override;

    TypeName(std::string name)
        : Token(TokenKind::TypeName), name(std::move(name)) {}

    TOK_CLASS(TypeName);
};

/**
 * @brief Non-type identifiers.
 */
struct Identifier: public Token {
    std::string name;

    virtual std::string repr(void) const override;

    Identifier(std::string name)
        : Token(TokenKind::Identifier), name(std::move(name)) {}

    TOK_CLASS(Identifier);
};

/**
 * @brief Signed integer literals.
 */
struct IntLiteral: public Token {
    int64_t value;

    virtual std::string repr(void) const override;

    IntLiteral(int64_t value): Token(TokenKind::IntLiteral), value(value) {}

    TOK_CLASS(IntLiteral);
};

/**
 * @brief Unsigned integer literals.
 */
struct UIntLiteral: public Token {
    uint64_t value;

    virtual std::string repr(void) const override;

    UIntLiteral(uint64_t value):
        Token(TokenKind::UIntLiteral),
        value(value) {}

    TOK_CLASS(UIntLiteral);
};

/**
 * @brief Floating-point literals.
 */
struct FloatLiteral: public Token {
    double value;

    virtual std::string repr(void) const override;

    FloatLiteral(double value):
        Token(TokenKind::FloatLiteral),
        value(value) {}

    TOK_CLASS(FloatLiteral);
};

/**
 * @brief String literals.
 */
struct StringLiteral: public Token {
    std::string value;

    virtual std::string repr(void) const override;

    StringLiteral(const std::string &value):
        Token(TokenKind::StringLiteral),
        value(value) {}

    TOK_CLASS(StringLiteral);
};

/**
 * @brief Operators.
 */
struct Operator: public Token {
    std::string op;

    virtual std::string repr(void) const override;

    Operator(std::string op): Token(TokenKind::Operator), op(std::move(op)) {}

    TOK_CLASS(Operator);
};

/**
 * @defgroup Empty Tokens with no meaning other than disambiguating syntax.
 */

#define TOK_SIMPLE(X)\
    X(): Token(TokenKind::X) {}\
    virtual bool operator==(const Token& other) const override {\
        return llvm::isa<X>(other);\
    }\
    static bool classof(const Token *t) {\
        return t->kind() == TokenKind::X;\
    }\
    ~X(void) override {}

struct OpenParen: public Token {
    virtual std::string repr(void) const override { return "("; }
    TOK_SIMPLE(OpenParen);
};
struct CloseParen: public Token {
    virtual std::string repr(void) const override { return ")"; }
    TOK_SIMPLE(CloseParen);
};
struct OpenBrace: public Token {
    virtual std::string repr(void) const override { return "{"; }
    TOK_SIMPLE(OpenBrace);
};
struct CloseBrace: public Token {
    virtual std::string repr(void) const override { return "}"; }
    TOK_SIMPLE(CloseBrace);
};
struct Comma: public Token {
    virtual std::string repr(void) const override { return ","; }
    TOK_SIMPLE(Comma);
};
struct Semicolon: public Token {
    virtual std::string repr(void) const override { return ";"; }
    TOK_SIMPLE(Semicolon);
};
struct Fn: public Token {
    virtual std::string repr(void) const override { return "fn"; }
    TOK_SIMPLE(Fn);
};
struct Struct: public Token {
    virtual std::string repr(void) const override { return "struct"; }
    TOK_SIMPLE(Struct);
};
struct Type: public Token {
    virtual std::string repr(void) const override { return "type"; }
    TOK_SIMPLE(Type);
};
struct Return: public Token {
    virtual std::string repr(void) const override { return "return"; }
    TOK_SIMPLE(Return);
};
struct If: public Token {
    virtual std::string repr(void) const override { return "if"; }
    TOK_SIMPLE(If);
};
struct Else: public Token {
    virtual std::string repr(void) const override { return "else"; }
    TOK_SIMPLE(Else);
};
struct While: public Token {
    virtual std::string repr(void) const override { return "while"; }
    TOK_SIMPLE(While);
};
struct InvalidToken: public Token {
    virtual std::string repr(void) const override { return "[INVALID]"; }
    TOK_SIMPLE(InvalidToken);
};

#undef TOK_CLASS
#undef TOK_SIMPLE

/** @} */

}

}
