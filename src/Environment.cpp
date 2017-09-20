/**
 * @file Environment.cpp
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

#include "Environment.hh"

namespace Craeft {

Type Variable::get_type(void) const {
    if (is_type<Function<>>(val.get_type())) {
        return val.get_type();
    }

    /* We actually only hold a *pointer* to the value, so we have to get
     * the pointed type. */
    return *boost::get<Pointer<> >(val.get_type()).get_pointed();
}

Environment::Environment(llvm::LLVMContext &ctx) {
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

void Environment::pop(void) {
    ident_map.pop();
    type_map.pop();
    template_map.pop();
    templatefunc_map.pop();
}

void Environment::push(void) {
    ident_map.push();
    type_map.push();
    template_map.push();
    templatefunc_map.push();
}

bool Environment::bound(const std::string &name) const {
    if (islower(name[0]) && ident_map.present(name)) {
        return true;
    } else {
        return type_map.present(name);
    }

    return false;
}

Variable Environment::lookup_identifier(const std::string &name,
                                        SourcePos pos) const {
    assert(!isupper(name[0]));

    try {
        return ident_map[name];
    } catch (KeyNotPresentException) {
        throw Error("name error", "variable \"" + name + "\" not found", pos);
    }
}

Variable Environment::add_identifier(std::string name, Value val) {
    Variable result(val);
    ident_map.bind(name, result);
    return result;
}

void Environment::add_type(std::string name, Type t) {
    type_map.bind(name, t);
}

const Type &Environment::lookup_type(const std::string &tname,
                                     SourcePos pos) const {
    assert(isupper(tname[0]));

    try {
        return type_map[tname];
    } catch (KeyNotPresentException) {
        throw Error("name error", "type \"" + tname + "\" not found", pos);
    }
}

const TemplateStruct &Environment::lookup_template(
        const std::string &tname, SourcePos pos) const {
    assert(isupper(tname[0]));

    try {
        return template_map[tname];
    } catch (KeyNotPresentException) {
        throw Error("name error", "template type \"" + tname + "\" not found",
                    pos);
    }
}

const TemplateValue &Environment::lookup_template_func(
        const std::string &func_name,
        SourcePos pos) const {
    assert(islower(func_name[0]));

    try {
        return templatefunc_map[func_name];
    } catch (KeyNotPresentException) {
        throw Error("name error", "template function \"" + func_name
                                + "\" not found", pos);
    }
}
}
