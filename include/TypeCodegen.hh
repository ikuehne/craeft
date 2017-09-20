/**
 * @file TypeCodegen.hh
 *
 * @brief Generating Types from AST expressions.
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

#include "AST/Types.hh"
#include "Translator.hh"

namespace Craeft {

/**
 * @brief Codegen for types: convert AST types to Craeft Types.
 */
class TypeCodegen: public AST::TypeVisitor<Type> {
public:
    TypeCodegen(Translator &translator): translator(translator) {}

    ~TypeCodegen(void) override {}

private:
    Type operator()(const AST::NamedType &) override;
    Type operator()(const AST::Void &) override;
    Type operator()(const AST::Pointer &) override;
    Type operator()(const AST::TemplatedType &) override;

    Translator &translator;
};

/**
 * @brief Codegen for template types: convert AST templates to TemplateTypes.
 */
class TemplateTypeCodegen: public AST::TypeVisitor<TemplateType> {
public:
    TemplateTypeCodegen(Translator &translator, std::vector<std::string> args)
        : translator(translator), args(args) {}

    ~TemplateTypeCodegen(void) override {}

private:
    TemplateType operator()(const AST::NamedType &) override;
    TemplateType operator()(const AST::Void &) override;
    TemplateType operator()(const AST::Pointer &) override;
    TemplateType operator()(const AST::TemplatedType &) override;

    Translator &translator;
    std::vector<std::string> args;
};

}
