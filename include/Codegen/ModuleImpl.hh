/**
 * @file Codegen/ModuleImpl.hh
 *
 * @brief Interface to the actual implementation of module codegen.
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

#include "llvm/ADT/Triple.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Target/TargetMachine.h"

#include "AST/Toplevel.hh"
#include "Environment.hh"
#include "Translator.hh"

namespace Craeft {

namespace Codegen {

/**
 * @brief Class containing actual implementation of ModuleGen methods.
 *
 * See Codegen/Module.hh for documentation of public methods.
 */
class ModuleGenImpl: public AST::ToplevelVisitor<void> {
public:
    ModuleGenImpl(std::string name, std::string triple, std::string fname);
    void validate(std::ostream &);
    void optimize(int opt_level);

    void emit_ir(std::ostream &);
    void emit_obj(int fd);
    void emit_asm(int fd);

    std::vector< std::pair< std::vector<Type>, TemplateValue> >
         codegen_function_with_name(
            const AST::FunctionDefinition &, std::string);

    // Visitors for top-level AST nodes.

private:
    void operator()(const AST::TypeDeclaration &) override;
    void operator()(const AST::StructDeclaration &) override;
    void operator()(const AST::TemplateStructDeclaration &) override;
    void operator()(const AST::FunctionDeclaration &) override;
    void operator()(const AST::FunctionDefinition &) override;
    void operator()(const AST::TemplateFunctionDefinition &) override;

    Translator _translator;

    /* Utilities. */
    Function<> type_of_ast_decl(const AST::FunctionDeclaration &fd);
};

}

}
