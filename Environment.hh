/**
 * @file Environment.hh
 *
 * @brief The Environment class, used for variable/type mappings.
 *
 * Contains convenience structs for holding information about named Craeft
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

namespace Craeft {

/**
 * @brief Information to be associated with a variable in an environment.
 */
struct IdentBinding {
    IdentBinding(void) {}

    /**
     * @brief Create a new `IdentBinding` based on the given instruction.
     */
    IdentBinding(llvm::Value *inst): inst(inst) {}

    /**
     * @brief A value corresponding to an LLVM pointer to this variable.
     *
     * For example, to get an instruction refering to the current value of
     * this variable, one would issue a "load" on this value.
     */
    llvm::Value *inst = NULL;

    llvm::Type *get_type(void) {
        // TODO: Explicitly handle case where this is not a pointer.  Should
        // never happen according to the current codegen design.
        return ((llvm::PointerType *)inst->getType())->getElementType();
    }
};

/**
 * @brief Information to be associated with a type environment.
 */
struct TypeBinding {
    TypeBinding(void) {}

    /**
     * @brief An LLVM type corresponding to the given type.
     */
    llvm::Type *type = NULL;

    TypeBinding(llvm::Type *type): type(type) {}

};


typedef std::vector<std::pair<std::string, IdentBinding> > IdentScope;
typedef std::vector<std::pair<std::string, TypeBinding> >  TypeScope;

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
     * @brief Find the given name in the map, or insert it if not found.
     *
     * @param name Must be a valid identifier.
     */
    IdentBinding &operator[](const std::string &name) {
        assert(islower(name[0]));

        // First try to find the result,
        for (auto &vec: boost::adaptors::reverse(ident_map)) {
            for (auto &pair: *vec) {
                if (pair.first == name) {
                    // returning it if possible.
                    return pair.second;
                }
            }
        }

        ident_map.back()->push_back(std::pair<std::string, IdentBinding>(
                    name, IdentBinding()));

        return ident_map.back()->back().second;
    }

    /**
     * @brief Find the given type name in the map, or insert it if not found.
     *
     * @param tname Must be a valid type name.
     */
    TypeBinding &operator()(const std::string &tname) {
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

        type_map.back()->push_back(std::pair<std::string, TypeBinding>(
                    tname, TypeBinding()));

        return type_map.back()->back().second;
    }

private:
    std::vector < std::unique_ptr <IdentScope> >ident_map;
    std::vector < std::unique_ptr <TypeScope> > type_map;
};

}
