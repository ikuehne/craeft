/**
 * @file Codegen/Type.cpp
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

#include "Codegen/Type.hh"

namespace Craeft {

namespace Codegen {

Type TypeGen::operator()(const AST::NamedType &it) {
    // TODO: Annotate type names with source positions.
    return translator.lookup_type(it.name(), it.pos());
}

Type TypeGen::operator()(const AST::Void &_) {
    return Void();
}

Type TypeGen::operator()(const AST::Pointer &ut) {
    return Pointer<>(visit(ut.pointed()));
}

Type TypeGen::operator()(const AST::TemplatedType &t) {
    std::vector<Type> args;

    for (const auto &arg: t.args()) {
        args.push_back(visit(*arg));
    }

    return translator.specialize_template(t.name(), args, t.pos());
}

/*****************************************************************************
 * Code generation for template types.
 */

TemplateType TemplateTypeGen::operator()(const AST::NamedType &it) {
    for (int i = 0; i < (int)args.size(); ++i) {
        if (it.name() == args[i]) {
            return i;
        }
    }
    
    return to_template(translator.lookup_type(it.name(), it.pos()));
}

TemplateType TemplateTypeGen::operator()(const AST::Void &_) {
    return Void();
}

TemplateType TemplateTypeGen::operator()(const AST::Pointer &ut) {
    return Pointer<TemplateType>(visit(ut.pointed()));
}

TemplateType TemplateTypeGen::operator()(const AST::TemplatedType &t) {
    std::vector<TemplateType> args;

    for (const auto &arg: t.args()) {
        args.push_back(visit(*arg));
    }

    return translator.respecialize_template(t.name(), args, t.pos());
}

}
}
