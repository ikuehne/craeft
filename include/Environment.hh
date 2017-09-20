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

#include "AST/Toplevel.hh"
#include "Error.hh"
#include "Type.hh"
#include "Scope.hh"
#include "Value.hh"
#include "VariantUtils.hh"

namespace Craeft {

/**
 * @brief Information to be associated with a variable in an environment.
 */
class Variable {
public:
    /**
     * @brief Create a new `Variable` based on the given instruction.
     */
    Variable(Value val): val(val) {}

    /**
     * @brief Get the type of this binding.
     */
    Type get_type(void) const;

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
    Environment(llvm::LLVMContext &ctx);

    /**
     * @brief Pop the most recently entered (deepest) scope.
     */
    void pop(void);

    /**
     * @brief Push a new, empty scope.
     */
    void push(void);

    /**
     * @brief Get whether the given name is bound in any scope.
     */
    bool bound(const std::string &name) const;

    /**
     * @brief Find the given name in the map.
     *
     * Throw an Error if not found.
     *
     * @param name Must be a valid identifier.
     */
    Variable lookup_identifier(const std::string &name, SourcePos pos) const;

    Variable add_identifier(std::string name, Value val);

    void add_type(std::string name, Type t);

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
    const Type &lookup_type(const std::string &tname, SourcePos pos) const;

    const TemplateStruct &lookup_template(const std::string &tname,
                                          SourcePos pos) const;

    const TemplateValue &lookup_template_func(const std::string &func_name,
                                              SourcePos pos) const;

private:
    Scope<Variable> ident_map;
    Scope<Type> type_map;
    Scope<TemplateStruct> template_map;
    Scope<TemplateValue> templatefunc_map;
};

}
