/**
 * @file Codegen/ModuleImpl.cpp
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

#include <algorithm>
#include <iostream>

#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"

#include "Codegen/ModuleImpl.hh"
#include "Codegen/Type.hh"
#include "Codegen/Statement.hh"

namespace Craeft {

namespace Codegen {

ModuleGenImpl::ModuleGenImpl(std::string name, std::string triple,
                                     std::string fname)
    : _translator(name, fname, triple) {
}

void ModuleGenImpl::emit_ir(std::ostream &out) {
    _translator.emit_ir(out);
}

void ModuleGenImpl::emit_asm(int fd) {
    _translator.emit_asm(fd);
}

void ModuleGenImpl::emit_obj(int fd) {
    _translator.emit_obj(fd);
}

void ModuleGenImpl::operator()(const AST::TypeDeclaration &td) {
    throw Error("error", "type declarations not implemented", td.pos());
}

void ModuleGenImpl::operator()(const AST::StructDeclaration &sd) {
    std::vector<std::pair<std::string, std::shared_ptr<Type> > >fields;
    TypeGen tg(_translator);

    for (const auto &decl: sd.members()) {
        auto t = std::make_shared<Type>(tg.visit(decl->type()));
        fields.push_back(std::pair<std::string, std::shared_ptr<Type> >
                                  (decl->name().name(), t));
    }

    Struct<> t(fields, sd.name());

    _translator.create_struct(t);
}

void ModuleGenImpl::operator()(const AST::TemplateStructDeclaration &s) {
    std::vector<std::pair<std::string, std::shared_ptr<TemplateType> > >fields;
    TemplateTypeGen tg(_translator, s.argnames());

    for (const auto &decl: s.decl().members()) {
        auto t = std::make_shared<TemplateType>(tg.visit(decl->type()));
        fields.push_back(std::pair<std::string,
                                  std::shared_ptr<TemplateType> >
                                  (decl->name().name(), t));
    }

    Struct<TemplateType> t(fields, s.decl().name());

    TemplateStruct tmpl(t, s.argnames().size());

    _translator.register_template(tmpl, s.decl().name());
}

Function<> ModuleGenImpl::type_of_ast_decl(
        const AST::FunctionDeclaration &fd) {
    std::vector<std::shared_ptr<Type> > arg_types;
    TypeGen tg(_translator);

    for (const auto &decl: fd.args()) {
        arg_types.push_back(std::make_shared<Type>(tg.visit(decl->type())));
    }

    auto ret_type = std::make_shared<Type>(tg.visit(fd.ret_type()));

    return Function<>(ret_type, arg_types);
}

void ModuleGenImpl::operator()(const AST::FunctionDeclaration &fd) {
    auto ty = type_of_ast_decl(fd);
    _translator.create_function_prototype(ty, fd.name());
}

std::vector< std::pair< std::vector<Type>, TemplateValue> >
ModuleGenImpl::codegen_function_with_name(
        const AST::FunctionDefinition &fd,
        std::string name) {
    auto ty = type_of_ast_decl(fd.signature());

    std::vector<std::string> arg_names;

    for (auto &decl: fd.signature().args()) {
        arg_names.push_back(decl->name().name());
    }

    _translator.create_and_start_function(ty, arg_names, name);

    for (const auto &arg: fd.block()) {
        StatementGen(_translator).visit(*arg);
    }

    return _translator.end_function();
}

void ModuleGenImpl::operator()(const AST::FunctionDefinition &fd) {
    auto specializations = codegen_function_with_name(fd,
                                                      fd.signature().name());

    for (int i = 0; i < (int)specializations.size(); ++i) {
        const auto specialization = specializations[i];
        const auto &args = specialization.first;
        const auto &val = specialization.second;

        assert(val.arg_names.size() == args.size());

        _translator.push_scope();

        for (int j = 0; j < (int)args.size(); ++j) {
            _translator.bind_type(val.arg_names[j], args[j]);
        }

        auto name = mangle_name(val.fd->signature().name(), args);

        // Add any specializations added in codegen for *this* specialization.
        auto new_specializations = codegen_function_with_name(*val.fd, name);

        for (const auto &specialization: new_specializations) {
            specializations.push_back(specialization);
        }

        _translator.pop_scope();
    }
}

void ModuleGenImpl::operator()(const AST::TemplateFunctionDefinition &f) {
    auto name = f.def()->signature().name();

    std::vector<std::shared_ptr<TemplateType> >arg_types;
    TemplateTypeGen tg(_translator, f.argnames());

    for (const auto &decl: f.def()->signature().args()) {
        arg_types.push_back(std::make_shared<TemplateType>
                                            (tg.visit(decl->type())));
    }

    auto ret_type = std::make_shared<TemplateType>
                                (tg.visit(f.def()->signature().ret_type()));

    auto t = Function<TemplateType>(ret_type, arg_types);

    _translator.register_template(name, f.def(), f.argnames(),
                                 TemplateFunction(t, f.argnames()));
}

void ModuleGenImpl::optimize(int opt_level) {
    _translator.optimize(opt_level);
}

void ModuleGenImpl::validate(std::ostream &out) {
    _translator.validate(out);
}

}
}
