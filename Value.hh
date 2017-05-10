/**
 * @file Value.hh
 *
 * Internal representation of Craeft values.
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

#include "Type.hh"

// Forward-declare llvm::Value.
namespace llvm {
    class Value;
}

namespace Craeft {

class Value {
public:
    /**
     * @brief Make a new Craeft value based on the given LLVM instruction and
     *        with the given Craeft type.
     *
     * @brief inst The LLVM instruction which produces the LLVM value
     *             corresponding to this Craeft value.
     *
     * @brief ty A Craeft type.  If the LLVM representation of this type is
     *           not the same as the type of `inst`, an assertion will be
     *           tripped.
     */
    Value(llvm::Value *inst, Type ty);

    /**
     * @brief Convert this Craeft value to an LLVM value.
     */
    llvm::Value *to_llvm(void) const { return inst; }

    bool is_integral(void);

    /**
     * @brief Get the Craeft type of this value.
     */
    const Type &get_type(void) const { return ty; }

private:
    llvm::Value *inst;
    Type ty;
};

}
