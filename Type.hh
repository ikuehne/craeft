/**
 * @file Type.hh
 *
 * @brief Types as represented in the compiler.
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
#include <memory>
#include <string>

#include <boost/variant.hpp>

namespace Craeft {

/**
 * @brief A signed integer type.
 */
struct IntType {
    int nbits;

    IntType(int nbits): nbits(nbits) {}
};

/**
 * @brief An unsigned integer type.
 */
struct UIntType {
    int nbits;

    UIntType(int nbits): nbits(nbits) {}
};

/**
 * @brief A single-precision float.
 */
struct Float {};

/**
 * @brief A double-precision float.
 */
struct Double {};

/**
 * @brief Any other type.
 */
struct UserType {
    std::string name;
    
    UserType(std::string n): name(n) {}
};

struct Pointer;

typedef boost::variant< IntType, UIntType, Float, Double, UserType,
                        std::unique_ptr<Pointer> > Type;

/**
 * @brief A pointer to another type.
 */
struct Pointer {
    Type pointed;
};

}
