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
#include <memory>
#include <string>
#include <vector>

#include <boost/range/adaptor/reversed.hpp>
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

#include "Error.hh"
#include "Type.hh"
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
        assert(is_type<Pointer>(val.get_type()));
    }

    /**
     * @brief Get the type of this binding.
     */
    Type *get_type(void) {
        /* We actually only hold a *pointer* to the value, so we have to get
         * the pointed type. */
        return boost::get<Pointer>(val.get_type()).get_pointed();
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

typedef std::vector<std::pair<std::string, Variable> > IdentScope;
typedef std::vector<std::pair<std::string, Type> >  TypeScope;

class Environment {
public:
    /**
     * @brief Create a new, empty environment.
     */
    Environment(void) {
        // Should always have at least one scope.
        push();
    }

    /**
     * @brief Pop the most recently entered (deepest) scope.
     */
    void pop(void) {
        // TODO: properly handle this error.
        assert(ident_map.size() > 1);

        ident_map.pop_back();
        type_map.pop_back();
    }

    /**
     * @brief Push a new, empty scope.
     */
    void push(void) {
        ident_map.push_back(std::make_unique<IdentScope>());
        type_map.push_back(std::make_unique<TypeScope>());
    }

    /**
     * @brief Get whether the given name is bound in any scope.
     */
    bool bound(const std::string &name) {
        if (islower(name[0])) {
            for (auto &vec: boost::adaptors::reverse(ident_map)) {
                for (auto &pair: *vec) {
                    if (pair.first == name) {
                        return true;
                    }
                }
            }
        } else if (isupper(name[0])) {
            for (auto &vec: boost::adaptors::reverse(type_map)) {
                for (auto &pair: *vec) {
                    if (pair.first == name) {
                        return true;
                    }
                }
            }
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
        assert(islower(name[0]));

        // First try to find the result,
        for (auto &vec: boost::adaptors::reverse(ident_map)) {
            for (auto &pair: *boost::adaptors::reverse(vec)) {
                if (pair.first == name) {
                    // returning it if possible.
                    return pair.second;
                }
            }
        }

        throw Error("error", "variable \"" + name + "\" not found.",
                    fname, pos);
    }

    Variable add_identifier(std::string name, Value val) {
        Variable result(val);
        ident_map.back()->push_back(std::pair<std::string, Variable>
                                             (name, result));
        return result;
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

        // First try to find the result,
        for (auto &vec: boost::adaptors::reverse(type_map)) {
            for (auto &pair: *vec) {
                if (pair.first == tname) {
                    // returning it if possible.
                    return pair.second;
                }
            }
        }

        throw Error("error", "type \"" + tname + "\" not found.",
                    fname, pos);
    }

private:
    std::vector < std::unique_ptr <IdentScope> > ident_map;
    std::vector < std::unique_ptr <TypeScope> > type_map;
};

}
