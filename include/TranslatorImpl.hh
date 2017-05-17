/**
 * @file TranslatorImpl.hh
 *
 * @brief Actual implementation of the Translator.
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

#include <functional>

#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"

#include "Block.hh"
#include "Environment.hh"
#include "Error.hh"
#include "Translator.hh"
#include "Type.hh"
#include "Value.hh"

namespace Craeft {

/**
 * @brief Implementation of the Translator.
 *
 * See Translator.hh for documentation of public members.
 */
class TranslatorImpl {
public:
    TranslatorImpl(std::string module_name, std::string filename,
                   std::string triple);

    Value cast(Value val, const Type &t, SourcePos pos);
    Value add_load(Value pointer, SourcePos pos);
    void add_store(Value pointer, Value, SourcePos pos);

    Value left_shift(Value val, Value nbits, SourcePos pos);
    Value right_shift(Value val, Value nbits, SourcePos pos);
    Value bit_and(Value lhs, Value rhs, SourcePos pos);
    Value bit_or(Value lhs, Value rhs, SourcePos pos);
    Value bit_xor(Value lhs, Value rhs, SourcePos pos);
    Value bit_not(Value val, SourcePos pos);
    Value add(Value lhs, Value rhs, SourcePos pos);
    Value sub(Value lhs, Value rhs, SourcePos pos);
    Value mul(Value lhs, Value rhs, SourcePos pos);
    Value div(Value lhs, Value rhs, SourcePos pos);
    Value equal(Value lhs, Value rhs, SourcePos pos);
    Value nequal(Value lhs, Value rhs, SourcePos pos);
    Value less(Value lhs, Value rhs, SourcePos pos);
    Value lesseq(Value lhs, Value rhs, SourcePos pos);
    Value greater(Value lhs, Value rhs, SourcePos pos);
    Value greatereq(Value lhs, Value rhs, SourcePos pos);
    Value bool_and(Value lhs, Value rhs, SourcePos pos);
    Value bool_or(Value lhs, Value rhs, SourcePos pos);
    Value bool_not(Value val, SourcePos pos);

    Value call(std::string func, std::vector<Value> &args, SourcePos pos);
    Variable declare(const std::string &name, const Type &t);
    void assign(const std::string &varname, Value val, SourcePos pos);
    void return_(Value val, SourcePos pos);
    void return_(SourcePos pos);

    Value get_identifier_addr(std::string ident, SourcePos pos);
    Value get_identifier_value(std::string ident, SourcePos pos);
    Type lookup_type(std::string tname, SourcePos pos);

    IfThenElse create_ifthenelse(Value cond, SourcePos pos);
    void point_to_else(IfThenElse &structure);
    void end_ifthenelse(IfThenElse structure);
    void create_function_prototype(Function f, std::string name);
    void create_and_start_function(Function f, std::vector<std::string> args,
                                   std::string name);

    void end_function(void);

    void validate(std::ostream &);
    void optimize(int opt_level);
    void emit_ir(std::ostream &);
    void emit_obj(int fd);
    void emit_asm(int fd);

    llvm::LLVMContext &get_ctx(void) { return context; }

private:
    Value apply_to_wider_integer(Value lhs, Value rhs, SourcePos pos,
                                 std::function<llvm::Value *(
                                        llvm::Value *, llvm::Value *)>);

    Value build_comp(Value lhs, Value rhs, SourcePos pos,
                     llvm::CmpInst::Predicate sip,
                     llvm::CmpInst::Predicate uip,
                     llvm::CmpInst::Predicate fp);

    /**
     * @brief Move to the other block.
     */
    void point(Block other);

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
     * @brief The block currently writing to.
     */
    std::unique_ptr<Block> current;
};

}
