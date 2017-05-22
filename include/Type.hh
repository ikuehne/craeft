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
#include <sstream>
#include <vector>

#include <boost/variant.hpp>

// Forward-declare LLVM things.
namespace llvm {
    class Type;
    class Module;
    class LLVMContext;
    class FunctionType;
    class StructType;
}

namespace Craeft {

/*****************************************************************************
 * Simple ("terminal") types.
 */

/**
 * @brief Signed integer types.
 */
class SignedInt {
public:
    /**
     * @brief Build a signed integer type of the given width.
     */
    SignedInt(int nbits);

    llvm::Type *to_llvm(llvm::LLVMContext &) const;

    bool operator==(const SignedInt &other) const;

    int get_nbits(void) const {
        return nbits;
    }

private:
    int nbits;
};

/**
 * @brief Unsigned integer types.
 */
class UnsignedInt {
public:
    /**
     * @brief Build an unsigned integer type of the given width in the given
     *        context.
     */
    UnsignedInt(int nbits);

    int get_nbits(void) const { return nbits; }

    llvm::Type *to_llvm(llvm::LLVMContext &) const;
    bool operator==(const UnsignedInt &other) const;

private:
    int nbits;
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
     * @brief Build a floating-point type of the given precision in the given
     *        context.
     */
    Float(Precision);

    bool operator<(const Float &other) const;
    bool operator==(const Float &other) const;

    Precision get_precision(void) const {
        return prec;
    }

    llvm::Type *to_llvm(llvm::LLVMContext &) const;

private:
    Precision prec;
};

class Void {
public:
    /**
     * @brief Get the void type for the given LLVM context.
     */
    Void(void);

    llvm::Type *to_llvm(llvm::LLVMContext &ctx) const;

    std::string get_name(void) const {
        return "void";
    }
    /**
     * @brief For consistency: all void types are equal.
     */
    bool operator==(const Void &other) const { return true; }
};

struct Type;

/*****************************************************************************
 * Generic, "nonterminal" types.
 */

/**
 * @brief Pointer types.
 */
template<typename TypeType=Type>
class Pointer {
public:
    /**
     * @brief Build a pointer type pointing to the given type.
     */
    Pointer(TypeType pointed)
        : pointed(std::make_shared<TypeType>(pointed)) {}

    Pointer(std::shared_ptr<TypeType> pointed): pointed(pointed) {}

    TypeType *get_pointed(void) const { return pointed.get(); }

    bool operator==(const Pointer<TypeType> &other) const {
        return pointed == other.pointed;
    }

private:
    std::shared_ptr<TypeType> pointed;
};

template<typename TypeType=Type>
class Function {
public:
    Function(std::shared_ptr<TypeType> rettype,
             std::vector<std::shared_ptr<TypeType> >args)
          : rettype(rettype), args(args) {
        assert(rettype.get());
    }

    bool operator==(const Function<TypeType> &other) const {
        // Check that return types are equal,
        if (*other.rettype != *rettype) {
            return false;
        }

        if (args.size() != other.args.size()) {
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

    TypeType *get_rettype(void) const {
        return rettype.get();
    }

    const std::vector<std::shared_ptr<TypeType> > &get_args(void) const {
        return args;
    }

private:
    std::shared_ptr<TypeType> rettype;
    std::vector<std::shared_ptr<TypeType> > args;
};

/**
 * @brief Craeft structs.
 */
template<typename TypeType=Type>
class Struct {
public:
    /**
     * @brief Create a new struct type.
     *
     * @param fields An array of field name/type pairs.
     */
    Struct(std::vector< std::pair< std::string, std::shared_ptr<TypeType> > >
                fields,
            std::string name)
          : fields(fields), name(name) {

    }

    const std::vector<std::pair<std::string, std::shared_ptr<TypeType> > > &
          get_fields(void) const {
        return fields;
    }

    std::string get_name(void) const {
        return name;
    }

    bool operator==(const Struct<TypeType> &other) const {
        if (fields.size() != other.fields.size()) {
            return false;
        }

        // Check that field types are equal.
        for (unsigned i = 0; i < fields.size(); ++i) {
            if ((fields[i].first != other.fields[i].first)
             || (*fields[i].second != other.fields[i].second)) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Get the type and index corresponding to the given field in this
     *        struct.
     *
     * Return `(-1, nullptr)` if no such field.
     */
    std::pair<int, TypeType *>operator[](std::string field_name) {
        for (int i = 0; i < (int)fields.size(); ++i) {
            const auto &pair = fields[i];
            if (pair.first == field_name) {
                return std::pair<int, TypeType *>(i, pair.second.get());
            }
        }

        return std::pair<int, TypeType *>(-1, NULL);
    }

private:
    std::vector< std::pair< std::string, std::shared_ptr<TypeType> > > fields;
    std::string name;
};


/*****************************************************************************
 * Completed types: types with no template parameters missing.
 */

typedef boost::variant<SignedInt, UnsignedInt, Float, Void, Pointer<Type>,
                       Function<Type>, Struct<Type> > _Type;

/**
 * @brief Internal representation of Craeft types.
 *
 * Both represents everything about Craeft types and allows for simple
 * translation to the corresponding LLVM type.
 */
struct Type: public _Type {
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
llvm::Type *to_llvm_type(const Type &t, llvm::Module &module);

std::string get_name(Type t);

/*****************************************************************************
 * Template types: types with template parameters potentially missing.
 */

struct TemplateType;

struct TemplateStruct {
    int n_parameters;

    Struct<TemplateType> inner;

    int get_nparameters(void) const;

    const Struct<TemplateType> &get_inner(void) const;
};

struct TemplateFunction {
    int n_parameters;

    Function<TemplateType> inner;

    int get_nparameters(void) const;

    const Function<TemplateType> &get_inner(void) const;
};

typedef boost::variant<SignedInt, UnsignedInt, Float, Void,
                       Pointer<TemplateType>, Struct<TemplateType>,
                       Function<TemplateType>, int>
        _TemplateType;

struct TemplateType: public _TemplateType {};

/**
 * @brief Specialize the given template type given a list of template
 *        arguments.
 */
Type specialize(const TemplateType &temp, const std::vector<Type> &args);

/**
 * @brief Mangle the name of the given template function given the provided
 *        template arguments.
 *
 * The function is pure and produces a label which cannot conflict with any
 * other value or type name.
 */
std::string mangle_name(std::string fname,
                        const std::vector<Type> &args);

}
