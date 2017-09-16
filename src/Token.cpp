/**
 * @file Token.cpp
 *
 * @brief Tokens as output by the lexer.
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

#include "Token.hh"

#include <sstream>

namespace {

template<typename T>
std::string to_string(const T& x) {
    std::stringstream stream;
    stream << x;
    std::string result;
    stream >> result;
    return result;
}

}

namespace Craeft {

namespace Tok {

std::string TypeName::repr(void) const {
    return name;
}

#define TOK_EQUALS(X, Y, BODY)\
    bool X::operator==(const Token& _TOK_EQUALS_OTHER) const {\
        if (auto *Y = llvm::dyn_cast<X>(&_TOK_EQUALS_OTHER)) {\
            return BODY;\
        }\
        return false;\
    }

TOK_EQUALS(TypeName, lhs, name == lhs->name);

std::string Identifier::repr(void) const {
    return name;
}

TOK_EQUALS(Identifier, lhs, name == lhs->name);

std::string IntLiteral::repr(void) const {
    return to_string(value);
}

TOK_EQUALS(IntLiteral, lhs, value == lhs->value);

std::string UIntLiteral::repr(void) const {
    return to_string(value);
}

TOK_EQUALS(UIntLiteral, lhs, value == lhs->value);

std::string FloatLiteral::repr(void) const {
    return to_string(value);
}

TOK_EQUALS(FloatLiteral, lhs, value == lhs->value);

std::string Operator::repr(void) const {
    return op;
}

TOK_EQUALS(Operator, lhs, op == lhs->op);

}

}
