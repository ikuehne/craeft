/**
 * @file Value.cpp
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

#include "Value.hh"
#include "VariantUtils.hh"

namespace Craeft {

Value::Value(llvm::Value *inst, Type ty): inst(inst), ty(ty) {
    //assert(inst->getType() == to_llvm_type(ty));
}

bool Value::is_integral(void) const {
    return is_type<SignedInt>(ty) || is_type<UnsignedInt>(ty);
}

}
