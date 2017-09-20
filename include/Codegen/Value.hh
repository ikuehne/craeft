/**
 * @file Codegen/Value.hh
 *
 * @brief Generating Values from AST expressions.
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

#include "AST/Expressions.hh"
#include "Translator.hh"
#include "Value.hh"

namespace Craeft {

namespace Codegen {

/**
 * @brief Codegen for l-values: return the address of the given AST l-value.
 *
 * For a variable, return the address of the stack space that variable
 * occupies; for a dereference, just return the address being dereferenced;
 * and for a FieldAccess, return the address of that field.
 */
class LValueGen: public AST::LValueVisitor<Value> {
public:
    LValueGen(Translator &translator): _translator(translator) {}

private:
    Value operator()(const AST::Variable &) override;
    Value operator()(const AST::Dereference &) override;
    Value operator()(const AST::FieldAccess &) override;

    Translator &_translator;
};

/**
 * @brief Codegen for r-values: return the value of the given AST r-value.
 */
class ValueGen: public AST::ExpressionVisitor<Value> {
public:
    ValueGen(Translator &translator)
        : _translator(translator), _ctx(_translator.get_ctx()) {}

private:
    Value operator()(const AST::IntLiteral &) override;
    Value operator()(const AST::UIntLiteral &) override;
    Value operator()(const AST::FloatLiteral &) override;
    Value operator()(const AST::StringLiteral &) override;
    Value operator()(const AST::Variable &) override;
    Value operator()(const AST::Reference &) override;
    Value operator()(const AST::Dereference &) override;
    Value operator()(const AST::FieldAccess &) override;
    Value operator()(const AST::Binop &) override;
    Value operator()(const AST::FunctionCall &) override;
    Value operator()(const AST::TemplateFunctionCall &) override;
    Value operator()(const AST::Cast &) override;

    Translator &_translator;
    llvm::LLVMContext &_ctx;
};

}
}
