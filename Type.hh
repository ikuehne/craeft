/**
 * @file Type.hh
 *
 * Internal representation of Craeft types.
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

#include <memory>
#include <vector>

#include <boost/variant.hpp>

// Forward-declare LLVM things.
namespace llvm {
    class Type;
    class LLVMContext;
}

namespace Craeft {

/**
 * @brief Signed integer types.
 */
class SignedInt {
public:
    /**
     * @brief Build a signed integer type corresponding to the given LLVM
     *        type.
     *
     * @param ty An LLVM integer type.  An assertion will be tripped if any
     *           other type is given.
     */
    SignedInt(llvm::Type *ty);

    /**
     * @brief Build a signed integer type of the given width in the given
     *        context.
     */
    SignedInt(int nbits, llvm::LLVMContext &);

    bool operator==(const SignedInt &other) const;
    llvm::Type *to_llvm(void) const;

private:
    llvm::Type *ty;
};

/**
 * @brief Unsigned integer types.
 */
class UnsignedInt {
public:
    /**
     * @brief Build an unsigned integer type corresponding to the given LLVM
     *        type.
     *
     * @param ty An LLVM integer type.  An assertion will be tripped if any
     *           other type is given.
     */
    UnsignedInt(llvm::Type *ty);

    /**
     * @brief Build an unsigned integer type of the given width in the given
     *        context.
     */
    UnsignedInt(int nbits, llvm::LLVMContext &);
    bool operator==(const UnsignedInt &other) const;
    llvm::Type *to_llvm(void) const;

private:
    llvm::Type *ty;
};

/**
 * @brief Possible floating-point precisions.
 */
enum Precision {
    /**
     * @brief Single precision (32 bits).
     */
    SinglePrecision,

    /**
     * @brief Double precision (64 bits).
     */
    DoublePrecision
};

/**
 * @brief Floating-point types.
 */
class Float {
public:
    /**
     * @brief Build a floating-point type corresponding to the given LLVM
     *        type.
     *
     * @param ty An LLVM float type.  An assertion will be tripped if any
     *           other type is given.
     */
    Float(llvm::Type *ty);

    /**
     * @brief Build a floating-point type of the given precision in the given
     *        context.
     */
    Float(Precision, llvm::LLVMContext &);

    bool operator<(const Float &other) const;
    bool operator==(const Float &other) const;
    llvm::Type *to_llvm(void) const;

private:
    llvm::Type *ty;
    Precision prec;
};

struct Type;

/**
 * @brief Pointer types.
 */
class Pointer {
public:
    /**
     * @brief Build a pointer type pointing to the given type.
     */
    Pointer(Type pointed);

    Type *get_pointed(void) const { return pointed.get(); }

    bool operator==(const Pointer &other) const;
    llvm::Type *to_llvm(void) const;

private:
    std::shared_ptr<Type> pointed;
};

class Function {
public:
    Function(std::shared_ptr<Type> rettype,
             std::vector<std::shared_ptr<Type> >args);

    bool operator==(const Function &other) const;
    Type *get_rettype(void) const { return rettype.get(); }
    const std::vector<std::shared_ptr<Type> > &get_args(void) const {
        return args;
    }
    llvm::FunctionType *to_llvm(void) const;

private:
    std::shared_ptr<Type> rettype;
    std::vector<std::shared_ptr<Type> > args;
};

typedef boost::variant<SignedInt, UnsignedInt, Float, Pointer, Function>
    _Type;

/**
 * @brief Internal representation of Craeft types.
 *
 * Both represents everything about Craeft types and allows for simple
 * translation to the corresponding LLVM type.
 */
struct Type: _Type {
    template<typename... Args>
    Type(Args... args): _Type(args...) {}

    bool operator==(const _Type &other) const {
        return (const _Type &)(*this) == other;
    } 

    bool operator!=(const Type &other) const {
        return !((const Type &)(*this) == other);
    } 
};

/**
 * @brief Convert the given type to a corresponding LLVM type.
 *
 * The LLVM type may encode less information, but all operations which are
 * possible on the Craeft type can be represented in LLVM on the returned
 * type.
 */
llvm::Type *to_llvm_type(const Type &t);

}
