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
 * Getting unique names for functions.
 */

struct NamingVisitor: public boost::static_visitor<std::string> {
    std::string operator()(const SignedInt &si) const {
        std::stringstream result;

        result << "signed";
        result << si.get_nbits();

        return result.str();
    }

    std::string operator()(const UnsignedInt &ui) const {
        std::stringstream result;

        result << "unsigned";
        result << ui.get_nbits();

        return result.str();
    }

    std::string operator()(const Float &f) const {
        if (f.get_precision() == SinglePrecision) {
            return "float";
        } else {
            return "double";
        }
    }

    std::string operator()(const Void &) const {
        return std::string("void");
    }

    std::string operator()(const Pointer<Type> &ptr) const {
        return std::string("$") + get_name(*ptr.get_pointed()) + "$";
    }

    std::string operator()(const Function<Type> &func) const {
        std::stringstream result;

        result << "$.";

        for (auto &arg: func.get_args()) {
            result << get_name(*arg);
            result << ".";
        }

        result << func.get_rettype();

        result << ".$";

        return result.str();
    }

    std::string operator()(const Struct<Type> &str) const {
        return str.get_name();
    }
};

std::string get_name(Type t) {
    return boost::apply_visitor(NamingVisitor(), t);
}

/*****************************************************************************
 * Specializing template types.
 */

struct SpecializerTypeVisitor: public boost::static_visitor<Type> {
    SpecializerTypeVisitor(const std::vector<Type> &template_args)
        : args(template_args) {}

    template<typename T>
    Type operator()(const T &simple) const {
        return simple;
    }

    Type operator()(const Pointer<TemplateType> &ptr) const {
        return Pointer<Type>(specialize(*ptr.get_pointed(), args));
    }

    Struct<Type> operator()(const Struct<TemplateType> &str) const {
        std::vector<std::pair<std::string,
                              std::shared_ptr<Type> > >fields;

        for (const auto &t_field: str.get_fields()) {
            fields.push_back(std::pair<std::string, std::shared_ptr<Type> >
                                      (t_field.first,
                                       std::make_shared<Type>(
                                         specialize(*t_field.second, args))));
        }

        std::stringstream namestream;

        namestream << "tmpl." << str.get_name();

        for (auto &arg: args) {
            namestream << "." << get_name(arg);
        }

        return Struct<Type>(fields, namestream.str());
    }

    Type operator()(const int &i) const {
        return args[i];
    }

    Function<Type> operator()(const Function<TemplateType> &fn) const {
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
    const std::vector<Type> &args;
};

/*****************************************************************************
 * Respecializing template types.
 */

struct RespecializerTypeVisitor: public boost::static_visitor<TemplateType> {
    RespecializerTypeVisitor(const std::vector<TemplateType> &template_args)
        : args(template_args) {}

    template<typename T>
    TemplateType operator()(const T &simple) const {
        return simple;
    }

    TemplateType operator()(const Pointer<TemplateType> &ptr) const {
        auto pointed = boost::apply_visitor(*this, *ptr.get_pointed());
        return Pointer<TemplateType>(std::make_shared<TemplateType>(pointed));
    }

    Struct<TemplateType> operator()(const Struct<TemplateType> &str) const {
        std::vector<std::pair<std::string,
                              std::shared_ptr<TemplateType> > >fields;

        for (const auto &t_field: str.get_fields()) {
            auto field = boost::apply_visitor(*this, *t_field.second);
            fields.push_back(std::pair<std::string,
                                        std::shared_ptr<TemplateType> >
                                      (t_field.first,
                                       std::make_shared<TemplateType>(
                                           field)));
        }

        return Struct<TemplateType>(fields, str.get_name());
    }

    TemplateType operator()(const int &i) const {
        return args[i];
    }

    TemplateType operator()(const Function<TemplateType> &fn) const {
        auto r = boost::apply_visitor(*this, *fn.get_rettype());
        auto rettype = std::make_shared<TemplateType>(r);

        std::vector<std::shared_ptr<TemplateType> >fn_args;

        for (const auto &arg: fn.get_args()) {
            auto a = boost::apply_visitor(*this, *arg);
            fn_args.push_back(std::make_shared<TemplateType>(a));
        }

        return Function<TemplateType>(rettype, fn_args);
    }

private:
    const std::vector<TemplateType> &args;
};

Type specialize(const TemplateType &temp, const std::vector<Type> &args) {
    return boost::apply_visitor(SpecializerTypeVisitor(args), temp);
}

std::string mangle_name(std::string fname,
                        const std::vector<Type> &args) {
    std::stringstream namestream;

    namestream << "FnTmpl." << fname;

    for (auto &arg: args) {
        namestream << "." << get_name(arg);
    }

    return namestream.str();
}

/*****************************************************************************
 * Converting normal types to template types.
 */

class TemplatizerVisitor: public boost::static_visitor<TemplateType> {
public:
    template<typename T>
    TemplateType operator()(const T &t) const {
        return t;
    }

    TemplateType operator()(const Struct<Type> &t) const {
        std::vector<std::pair<std::string, std::shared_ptr<TemplateType> > >
                fields;

        for (const auto &field: t.get_fields()) {
            auto t = boost::apply_visitor(*this, *field.second);
            auto p = std::make_shared<TemplateType>(t);

            fields.push_back(
                    std::pair<std::string, std::shared_ptr<TemplateType> >(
                        field.first, p));
        }

        return Struct<TemplateType>(fields, t.get_name());
    }

    TemplateType operator()(const Pointer<Type> &p) const {
        auto pointed = boost::apply_visitor(*this, *p.get_pointed());

        auto ptr = std::make_shared<TemplateType>(pointed);

        return Pointer<TemplateType>(ptr);
    }

    Function<TemplateType> operator()(const Function<Type> &f) const {
        auto rettype = boost::apply_visitor(*this, *f.get_rettype());
        auto ptr = std::make_shared<TemplateType>(rettype);

        std::vector<std::shared_ptr<TemplateType> >args;

        for (const auto &arg: f.get_args()) {
            auto a = boost::apply_visitor(*this, *arg);
            args.push_back(std::make_shared<TemplateType>(a));
        }

        return Function<TemplateType>(ptr, args);
    }

};

TemplateType to_template(const Type &t) {
    return boost::apply_visitor(TemplatizerVisitor(), t);
}

/*****************************************************************************
 * Template types.
 */

TemplateStruct::TemplateStruct(Struct<TemplateType> inner, int n_parameters)
    : n_parameters(n_parameters), inner(inner) {}

int TemplateStruct::get_nparameters(void) const {
    return n_parameters;
}

const Struct<TemplateType> &TemplateStruct::get_inner(void) const {
    return inner;
}

Struct<Type> TemplateStruct::specialize(
        const std::vector<Type> &args) const {
    return SpecializerTypeVisitor(args)(get_inner());
}

Struct<TemplateType> TemplateStruct::respecialize(
        const std::vector<TemplateType> &args) const {
    return RespecializerTypeVisitor(args)(get_inner());
}

TemplateFunction::TemplateFunction(Function<TemplateType> inner,
                                   std::vector<std::string> args)
    : n_parameters(args.size()),
      inner(inner) {}

Function<Type> TemplateFunction::specialize(const std::vector<Type> &args) const {
    return SpecializerTypeVisitor(args)(inner);
}

}
