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

namespace Craeft {

class TypeCodegen: public boost::static_visitor<llvm::Type *> {
public:
    TypeCodegen(llvm::LLVMContext &context,
                llvm::IRBuilder<> &builder,
                llvm::Module &module,
                Environment &env,
                std::string &fname)
        : context(context), env(env), fname(fname) {}
    /**
     * @brief Get the LLVM type corresponding to the given Craeft type.
     */
    llvm::Type *codegen(const AST::Type &);

    // Visitors for Type nodes.
    llvm::Type *operator()(const AST::IntType &);
    llvm::Type *operator()(const AST::UIntType &);
    llvm::Type *operator()(const AST::Float &);
    llvm::Type *operator()(const AST::Double &);
    llvm::Type *operator()(const AST::Void &);
    llvm::Type *operator()(const AST::UserType &);
    llvm::Type *operator()(const std::unique_ptr<AST::Pointer> &);

private:
    llvm::LLVMContext &context;
    Environment &env;
    std::string &fname;
};

class ExpressionCodegen: public boost::static_visitor<llvm::Value *> {
public:
    ExpressionCodegen(llvm::LLVMContext &context,
                      llvm::IRBuilder<> &builder,
                      llvm::Module &module,
                      Environment &env,
                      std::string &fname)
        : context(context), builder(builder), module(module), env(env),
          fname(fname) {}

    /**
     * @brief Generate code for the given statement.
     */
    llvm::Value *codegen(const AST::Expression &expr);

    // Visitors of different AST statement types.
    llvm::Value *operator()(const AST::IntLiteral &);
    llvm::Value *operator()(const AST::UIntLiteral &);
    llvm::Value *operator()(const AST::FloatLiteral &);
    llvm::Value *operator()(const AST::Variable &);
    llvm::Value *operator()(const std::unique_ptr<AST::Binop> &);
    llvm::Value *operator()(const std::unique_ptr<AST::FunctionCall> &);
    llvm::Value *operator()(const std::unique_ptr<AST::Cast> &);

private:
    llvm::LLVMContext &context;
    llvm::IRBuilder<> &builder;
    llvm::Module &module;
    Environment &env;
    std::string &fname;
};

class StatementCodegen: public boost::static_visitor<void> {
public:
    /**
     * @brief Generate code for the given statement.
     */
    void codegen(const AST::Statement &stmt);

    StatementCodegen(llvm::LLVMContext &context,
                     llvm::IRBuilder<> &builder,
                     llvm::Module &module,
                     Environment &env,
                     std::string &fname,
                     llvm::Type *ret_type = NULL)
        : context(context), builder(builder), module(module), env(env),
          fname(fname), ret_type(ret_type),
          expr_codegen(context, builder, module, env, fname) {}

    void set_rettype(llvm::Type *ret);

    /**
     * @brief Return whether the last statement in the current block is
     *        unterminated.
     */
    bool can_continue(void);

    // Visitors of different AST statement types.
    void operator()(const std::unique_ptr<AST::Expression> &);
    void operator()(const AST::Return &);
    void operator()(const std::unique_ptr<AST::Declaration> &);
    void operator()(const std::unique_ptr<AST::CompoundDeclaration> &);
    void operator()(const std::unique_ptr<AST::IfStatement> &);

private:
    llvm::LLVMContext &context;
    llvm::IRBuilder<> &builder;
    llvm::Module &module;
    Environment &env;
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

    // Visitors for top-level AST nodes.
    void operator()(const AST::TypeDeclaration &);
    void operator()(const AST::StructDeclaration &);
    void operator()(const AST::FunctionDeclaration &);
    void operator()(const std::unique_ptr<AST::FunctionDefinition> &);

private:

    /**
     * @brief The name of the file this is generating code for.
     *
     * For error messages.
     */
    std::string fname;

    /**
     * @brief The compilation context.
     *
     * Essentially holds all LLVM state not particular to a module.
     */
    llvm::LLVMContext context;

    /**
     * @brief LLVM's helper for emitting IR.
     */
    llvm::IRBuilder<> builder;

    /**
     * @brief The module we are constructing.
     */
    std::unique_ptr<llvm::Module> module;

    /**
     * @brief Current namespace.
     */
    Environment env;

    /**
     * @brief The target machine (target triple + CPU information).
     */
	llvm::TargetMachine *target;

    /**
     * @brief The code generator for statements.
     */
    StatementCodegen stmt_cg;
};

}
