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
#include "llvm/IR/Module.h"

#include "Type.hh"

namespace Craeft {

/*****************************************************************************
 * Integer types.
 */

SignedInt::SignedInt(int nbits): nbits(nbits) {}

llvm::Type *SignedInt::to_llvm(llvm::LLVMContext &ctx) const {
    return llvm::IntegerType::get(ctx, nbits);
}

bool SignedInt::operator==(const SignedInt &other) const {
    return nbits == other.nbits;
}

UnsignedInt::UnsignedInt(int nbits)
    : nbits(nbits) {}

llvm::Type *UnsignedInt::to_llvm(llvm::LLVMContext &ctx) const {
    return llvm::IntegerType::get(ctx, nbits);
}

bool UnsignedInt::operator==(const UnsignedInt &other) const {
    return nbits == other.nbits;
}

/*****************************************************************************
 * @brief Floating-point types.
 */

Float::Float(Precision pr): prec(pr) {}

llvm::Type *Float::to_llvm(llvm::LLVMContext &ctx) const {
    switch (prec) {
        case SinglePrecision:
            return llvm::Type::getFloatTy(ctx);
            break;

        case DoublePrecision:
            return llvm::Type::getDoubleTy(ctx);
            break;
    }
}

bool Float::operator<(const Float &other) const {
    return prec < other.prec;
}

bool Float::operator==(const Float &other) const {
    return prec == other.prec;
}

/*****************************************************************************
 * Void types.
 */

Void::Void(void) {}

llvm::Type *Void::to_llvm(llvm::LLVMContext &ctx) const {
    return llvm::Type::getVoidTy(ctx);
}

/*****************************************************************************
 * Conversion to LLVM.
 */

/* Visitor for converting Craeft types to LLVM types. */
struct ToLlvmVisitor: public boost::static_visitor<llvm::Type *> {
    ToLlvmVisitor(llvm::Module &mod): ctx(mod.getContext()),
                                      mod(mod) {}

    template<typename T>
    llvm::Type *operator()(const T &t) const {
        return t.to_llvm(ctx);
    }

    llvm::Type *operator()(const Function<Type> &func) const {
        std::vector<llvm::Type *> arg_types;

        for (const auto &t: func.get_args()) {
            arg_types.push_back(to_llvm_type(*t, mod));
        }

        auto *ret = to_llvm_type(*func.get_rettype(), mod);

        return llvm::FunctionType::get(ret, arg_types, false);
    }

    llvm::Type *operator()(const Pointer<Type> &ptr) const {
        return llvm::PointerType::getUnqual(to_llvm_type(*ptr.get_pointed(),
                                            mod));
    }

    llvm::Type *operator()(const Struct<Type> &str) const {
        auto *result = mod.getTypeByName(str.get_name());
        if (result) {
            return result;
        }

        std::vector<llvm::Type *> arg_types;

        for (const auto &field: str.get_fields()) {
            arg_types.push_back(to_llvm_type(*field.second, mod));
        }

        result = llvm::StructType::create(arg_types, str.get_name());

        return result;
    }

private:
    llvm::LLVMContext &ctx;
    llvm::Module &mod;
};

llvm::Type *to_llvm_type(const Type &t, llvm::Module &mod) {
    return boost::apply_visitor(ToLlvmVisitor(mod), t);
}

/*****************************************************************************
 * Specializing template types.
 */

struct SpecializerTypeVisitor: public boost::static_visitor<Type> {
    SpecializerTypeVisitor(std::vector<Type> &template_args)
        : args(template_args) {}

    template<typename T>
    Type operator()(const T &simple) const {
        return simple;
    }

    Type operator()(const Pointer<TemplateType> &ptr) const {
        return Pointer<Type>(specialize(*ptr.get_pointed(), args));
    }

    Type operator()(const Struct<TemplateType> &str) const {
        std::vector<std::pair<std::string,
                              std::shared_ptr<Type> > >fields;

        for (const auto &t_field: str.get_fields()) {
            fields.push_back(std::pair<std::string, std::shared_ptr<Type> >
                                      (t_field.first,
                                       std::make_shared<Type>(
                                         specialize(*t_field.second, args))));
        }

        return Struct<Type>(fields, str.get_name());
    }

    Type operator()(const int &i) const {
        return args[i];
    }

    Type operator()(const Function<TemplateType> &fn) const {
        auto rettype = std::make_shared<Type>
                                       (specialize(*fn.get_rettype(), args));

        std::vector<std::shared_ptr<Type> >fn_args;

        for (const auto &arg: fn.get_args()) {
            fn_args.push_back(std::make_shared<Type>
                                              (specialize(*arg, args)));
        }

        return Function<Type>(rettype, fn_args);
    }

private:
    std::vector<Type> &args;
};

Type specialize(const TemplateType &temp, std::vector<Type> &args) {
    return boost::apply_visitor(SpecializerTypeVisitor(args), temp);
}

}
