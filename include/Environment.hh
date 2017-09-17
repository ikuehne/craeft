/**
 * @file Environment.hh
 *
 * @brief The Environment class, used for variable/type mappings.
 *
 * Contains a convenience struct for holding information about named Craeft
 * objects (variables and functions, for instance).  Also contains a class for
 * pushing and popping scopes.
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

#include <cctype>
#include <iostream>
#include <memory>
#include <string>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

#include "AST.hh"
#include "Error.hh"
#include "Type.hh"
#include "Scope.hh"
#include "Value.hh"
#include "VariantUtils.hh"

namespace Craeft {

/**
 * @brief Information to be associated with a variable in an environment.
 */
struct Variable {
    /**
     * @brief Create a new `Variable` based on the given instruction.
     */
    Variable(Value val): val(val) {
        /* Must be a pointer type.  All variables live in memory during
         * translation. */
        //assert(is_type<Pointer>(val.get_type()));
    }

    /**
     * @brief Get the type of this binding.
     */
    Type get_type(void) {
        if (is_type<Function<> >(val.get_type())) {
            return val.get_type();
        }

        /* We actually only hold a *pointer* to the value, so we have to get
         * the pointed type. */
        return *boost::get<Pointer<> >(val.get_type()).get_pointed();
    }

    /**
     * @brief Get the value of this variable.
     *
     * Note that the returned value corresponds to a *pointer* to the actual
     * contents of the variable.
     */
    Value get_val(void) { return val; }

private:
    /**
     * @brief A value corresponding to a pointer to this variable.
     *
     * To get an instruction refering to the current value of this variable,
     * one would issue a "load" on this value.
     */
    Value val;
};

struct TemplateValue {
    /**
     * @brief Create a new `Variable` based on the given instruction.
     */
    TemplateValue(std::shared_ptr<AST::FunctionDefinition> ast,
                  std::vector<std::string> arg_names,
                  TemplateFunction ty)
          : fd(ast), ty(ty), arg_names(arg_names) {} 

    /**
     * @brief The AST for this template function.
     *
     * We cannot actually compile this AST until we get the type arguments.
     */
    std::shared_ptr<AST::FunctionDefinition> fd;

    TemplateFunction ty;

    std::vector<std::string> arg_names;
};

class Environment {
public:
    /**
     * @brief Create a new, empty environment.
     */
    Environment(llvm::LLVMContext &ctx) {
        // Should always have at least one scope.
        push();

        // Add all of the built-in types.
        add_type("Float", Float(SinglePrecision));
        add_type("Double", Float(DoublePrecision));

        for (int i = 1; i <= 64; ++i) {
            add_type("I" + std::to_string(i), SignedInt(i));
            add_type("U" + std::to_string(i), UnsignedInt(i));
        }
    }

    /**
     * @brief Pop the most recently entered (deepest) scope.
     */
    void pop(void) {
        ident_map.pop();
        type_map.pop();
        template_map.pop();
        templatefunc_map.pop();
    }

    /**
     * @brief Push a new, empty scope.
     */
    void push(void) {
        ident_map.push();
        type_map.push();
        template_map.push();
        templatefunc_map.push();
    }

    /**
     * @brief Get whether the given name is bound in any scope.
     */
    bool bound(const std::string &name) {
        if (islower(name[0]) && ident_map.present(name)) {
            return true;
        } else {
            return type_map.present(name);
        }

        return false;
    }

    /**
     * @brief Find the given name in the map.
     *
     * Throw an Error if not found.
     *
     * @param name Must be a valid identifier.
     */
    Variable lookup_identifier(const std::string &name,
                               SourcePos pos, std::string &fname) {
        assert(!isupper(name[0]));

        try {
            return ident_map[name];
        } catch (KeyNotPresentException) {
            throw Error("name error", "variable \"" + name + "\" not found",
                        fname, pos);
        }
    }

    Variable add_identifier(std::string name, Value val) {
        Variable result(val);
        ident_map.bind(name, result);
        return result;
    }

    void add_type(std::string name, Type t) {
        type_map.bind(name, t);

        std::string fname("hello, there!");
        lookup_type(name, SourcePos(0, 0), fname);
    }

    void add_template_type(std::string name, TemplateStruct t) {
        template_map.bind(name, t);
    }

    void add_template_func(std::string name, TemplateValue v) {
        templatefunc_map.bind(name, v);
    }

    /**
     * @brief Find the given type name in the map.
     *
     * @param tname Must be a valid type name.
     */
    const Type &lookup_type(const std::string &tname,
                            SourcePos pos,
                            std::string &fname) {
        assert(isupper(tname[0]));

        try {
            return type_map[tname];
        } catch (KeyNotPresentException) {
            throw Error("name error", "type \"" + tname + "\" not found",
                        fname, pos);
        }
    }

    const TemplateStruct &lookup_template(const std::string &tname,
                                          SourcePos pos,
                                          std::string &fname) {
        assert(isupper(tname[0]));

        try {
            return template_map[tname];
        } catch (KeyNotPresentException) {
            throw Error("name error", "template type \"" + tname + "\" not "
                        "found",
                        fname, pos);
        }
    }

    const TemplateValue &lookup_template_func(const std::string &func_name,
                                              SourcePos pos,
                                              std::string &fname) {
        assert(islower(func_name[0]));

        try {
            return templatefunc_map[func_name];
        } catch (KeyNotPresentException) {
            throw Error("name error", "template function \"" + func_name
                                    + "\" not found", fname, pos);
        }
    }

private:
    Scope<Variable> ident_map;
    Scope<Type> type_map;
    Scope<TemplateStruct> template_map;
    Scope<TemplateValue> templatefunc_map;
};

}
