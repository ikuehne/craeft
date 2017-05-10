/**
 * @file Type.cpp
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

#include "llvm/IR/DerivedTypes.h"

#include "Type.hh"

namespace Craeft {

/*****************************************************************************
 * Integer types.
 */

SignedInt::SignedInt(llvm::Type *ty): ty(ty) {
    assert(ty->isIntegerTy());
}

SignedInt::SignedInt(int nbits, llvm::LLVMContext &ctx)
    : ty(llvm::IntegerType::get(ctx, nbits)) {}

llvm::Type *SignedInt::to_llvm(void) const {
    return ty;
}

bool SignedInt::operator==(const SignedInt &other) const {
    return ty == other.ty;
}

UnsignedInt::UnsignedInt(llvm::Type *ty): ty(ty) {
    assert(ty->isIntegerTy());
}

UnsignedInt::UnsignedInt(int nbits, llvm::LLVMContext &ctx)
    : ty(llvm::IntegerType::get(ctx, nbits)) {}

llvm::Type *UnsignedInt::to_llvm(void) const {
    return ty;
}

bool UnsignedInt::operator==(const UnsignedInt &other) const {
    return ty == other.ty;
}

/*****************************************************************************
 * @brief Floating-point types.
 */

Float::Float(llvm::Type *ty): ty(ty) {
    if (ty->isFloatTy()) {
        prec = SinglePrecision;
    } else if (ty->isDoubleTy()) {
        prec = DoublePrecision;
    } else {
        assert(0);
    }
}

Float::Float(Precision pr, llvm::LLVMContext &ctx): prec(pr) {
    switch (pr) {
        case SinglePrecision:
            ty = llvm::Type::getFloatTy(ctx);
            break;

        case DoublePrecision:
            ty = llvm::Type::getDoubleTy(ctx);
            break;
    }
}

llvm::Type *Float::to_llvm(void) const {
    return ty;
}

bool Float::operator<(const Float &other) const {
    return prec < other.prec;
}

bool Float::operator==(const Float &other) const {
    return ty == other.ty;
}

/*****************************************************************************
 * Pointer types.
 */

Pointer::Pointer(Type pointed): pointed(std::make_shared<Type>(pointed)) {}

llvm::Type *Pointer::to_llvm() const {
    return llvm::PointerType::getUnqual(Craeft::to_llvm_type(*pointed));
}

bool Pointer::operator==(const Pointer &other) const {
    return pointed == other.pointed;
}

/*****************************************************************************
 * Function types.
 */

Function::Function(std::shared_ptr<Type> rettype,
                   std::vector< std::shared_ptr<Type> >args)
    : rettype(rettype), args(args) {}

bool Function::operator==(const Function &other) const {
    // Check that return types are equal,
    if (*other.rettype != *rettype) {
        return false;
    }

    // and argument types are equal.
    for (unsigned i = 0; i < args.size(); ++i) {
        if (*args[i] != *other.args[i]) {
            return false;
        }
    }

    return true;
}

llvm::FunctionType *Function::to_llvm(void) const {
    std::vector<llvm::Type *> arg_types;

    for (const auto &t: args) {
        arg_types.push_back(to_llvm_type(*t));
    }

    auto *ret = to_llvm_type(*rettype);

    return llvm::FunctionType::get(ret, arg_types, false);
}

/*****************************************************************************
 * Conversion to LLVM.
 */

/* Visitor for converting Craeft types to LLVM types. */
struct ToLlvmVisitor: public boost::static_visitor<llvm::Type *> {
    template<typename T>
    llvm::Type *operator()(const T &t) const {
        return t.to_llvm();
    }
};

llvm::Type *to_llvm_type(const Type &t) {
    return boost::apply_visitor(ToLlvmVisitor(), t);
}

}
