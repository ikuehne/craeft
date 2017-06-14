/**
 * @file ModuleCodegenImpl.hh
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

#include "AST.hh"
#include "Environment.hh"
#include "Translator.hh"

namespace Craeft {

class TypeCodegen: public boost::static_visitor<Type> {
public:
    TypeCodegen(Translator &translator,
                std::string &fname)
        : translator(translator), fname(fname) {}
    /**
     * @brief Get the LLVM type corresponding to the given Craeft type.
     */
    Type codegen(const AST::Type &);

    // Visitors for Type nodes.
    Type operator()(const AST::NamedType &);
    Type operator()(const AST::Void &);
    Type operator()(const std::unique_ptr<AST::Pointer> &);
    Type operator()(const std::unique_ptr<AST::TemplatedType> &);

private:
    Translator &translator;
    std::string &fname;
};

class TemplateTypeCodegen: public boost::static_visitor<TemplateType> {
public:
    TemplateTypeCodegen(Translator &translator,
                        std::string &fname,
                        std::vector<std::string> args)
        : translator(translator), fname(fname), args(args) {}
    /**
     * @brief Get the LLVM type corresponding to the given Craeft type.
     */
    TemplateType codegen(const AST::Type &);

    // Visitors for Type nodes.
    TemplateType operator()(const AST::NamedType &);
    TemplateType operator()(const AST::Void &);
    TemplateType operator()(const std::unique_ptr<AST::Pointer> &);
    TemplateType operator()(const std::unique_ptr<AST::TemplatedType> &);

private:
    Translator &translator;
    std::vector<std::string> args;
    std::string &fname;
};

class ExpressionCodegen;

class LValueCodegen: public boost::static_visitor<Value> {
public:
    LValueCodegen(Translator &translator,
                  std::string &fname,
                  ExpressionCodegen &eg)
        : translator(translator),
          fname(fname), eg(eg) {}
    /**
     * @brief Generate an instruction yielding an address to the given
     *        l-value.
     */
    Value codegen(const AST::LValue &);

    Value operator()(const AST::Variable &);
    Value operator()(const std::unique_ptr<AST::Dereference> &);
    Value operator()(const std::unique_ptr<AST::FieldAccess> &);

private:
    Translator &translator;
    std::string &fname;
    ExpressionCodegen &eg;
};

class ExpressionCodegen: public boost::static_visitor<Value> {
public:
    ExpressionCodegen(Translator &translator,
                      std::string &fname)
        : translator(translator), fname(fname) {}

    /**
     * @brief Generate code for the given statement.
     */
    Value codegen(const AST::Expression &expr);

    // Visitors of different AST statement types.
    Value operator()(const AST::IntLiteral &);
    Value operator()(const AST::UIntLiteral &);
    Value operator()(const AST::FloatLiteral &);
    Value operator()(const AST::Variable &);
    Value operator()(const std::unique_ptr<AST::Reference> &);
    Value operator()(const std::unique_ptr<AST::Dereference> &);
    Value operator()(const std::unique_ptr<AST::Binop> &);
    Value operator()(const std::unique_ptr<AST::FunctionCall> &);
    Value operator()(const std::unique_ptr<AST::TemplateFunctionCall> &);
    Value operator()(const std::unique_ptr<AST::Cast> &);

private:
    Translator &translator;
    std::string &fname;
};

class StatementCodegen: public boost::static_visitor<void> {
public:
    /**
     * @brief Generate code for the given statement.
     */
    void codegen(const AST::Statement &stmt);

    StatementCodegen(Translator &translator,
                     std::string &fname)
        : translator(translator),
          fname(fname),
          expr_codegen(translator, fname) {}

    // Visitors of different AST statement types.
    void operator()(const AST::Expression &);
    void operator()(const AST::VoidReturn &);
    void operator()(const AST::Return &);
    void operator()(const std::unique_ptr<AST::Assignment> &assignment);
    void operator()(const std::unique_ptr<AST::Declaration> &);
    void operator()(const std::unique_ptr<AST::CompoundDeclaration> &);
    void operator()(const std::unique_ptr<AST::IfStatement> &);

private:
    Translator &translator;

    std::string &fname;
    llvm::Type *ret_type;

    /**
     * @brief The code generator for expressions.
     */
    ExpressionCodegen expr_codegen;
};

/**
 * @brief Class containing actual implementation of ModuleCodegen methods.
 *
 * See ModuleCodegen.hh for documentation of public methods.
 */
class ModuleCodegenImpl: public boost::static_visitor<void> {
public:
    ModuleCodegenImpl(std::string name, std::string triple,
                      std::string fname);
    void codegen(const AST::TopLevel &);

    void validate(std::ostream &);
    void optimize(int opt_level);

    void emit_ir(std::ostream &);
    void emit_obj(int fd);
    void emit_asm(int fd);

    std::vector< std::pair< std::vector<Type>, TemplateValue> >
         codegen_function_with_name(
            const AST::FunctionDefinition &, std::string);

    // Visitors for top-level AST nodes.
    void operator()(const AST::TypeDeclaration &);
    void operator()(const AST::StructDeclaration &);
    void operator()(const AST::TemplateStructDeclaration &);
    void operator()(const AST::FunctionDeclaration &);
    void operator()(const std::unique_ptr<AST::FunctionDefinition> &);
    void operator()(const AST::TemplateFunctionDefinition &);

private:
    Translator translator;

    /**
     * @brief The name of the file this is generating code for.
     *
     * For error messages.
     */
    std::string fname;

    /**
     * @brief The code generator for statements.
     */
    StatementCodegen stmt_cg;

    /* Utilities. */
    Function<> type_of_ast_decl(const AST::FunctionDeclaration &fd);
};

}
