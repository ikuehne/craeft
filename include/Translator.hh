/**
 * @file Translator.hh
 *
 * @brief Facilitates translation from Craeft to LLVM.
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

#include "llvm/Support/Host.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

#include "Environment.hh"
#include "Value.hh"
#include "Type.hh"

namespace Craeft {

class TranslatorImpl;

/**
 * @brief Facilities for translating Craeft to LLVM.
 *
 * Based off of, and tightly composed with, the LLVM IRBuilder.  Provides
 * primitive Craeft operations, which translate to LLVM instructions.
 */
class Translator {
public:
    Translator(std::string module_name, std::string filename,
               std::string triple=llvm::sys::getDefaultTargetTriple());

    ~Translator();

    /**
     * @defgroup Craeft instructions.
     *
     * Essentially abstract away LLVM's extremely strict typing.
     *
     * @{
     */

    /**
     * @brief Cast the given value to the given type.
     */
    Value cast(Value val, const Type &t, SourcePos pos);

    /**
     * @brief Dereference the given pointer.
     */
    Value add_load(Value pointer, SourcePos pos);

    void add_store(Value pointer, Value new_val, SourcePos pos);

    /**
     * @brief Left shift the given value by the given number of bits.
     */
    Value left_shift(Value val, Value nbits, SourcePos pos);

    /**
     * @brief Right shift the given value by the given number of bits.
     */
    Value right_shift(Value val, Value nbits, SourcePos pos);

    /**
     * @brief Bitwise AND the given values.
     */
    Value bit_and(Value lhs, Value rhs, SourcePos pos);

    /**
     * @brief Bitwise OR the given values.
     */
    Value bit_or(Value lhs, Value rhs, SourcePos pos);

    /**
     * @brief Bitwise XOR the given values.
     */
    Value bit_xor(Value lhs, Value rhs, SourcePos pos);

    /**
     * @brief Get the bitwise inverse of the given value.
     */
    Value bit_not(Value val, SourcePos pos);

    /**
     * @brief Add the given values.
     */
    Value add(Value lhs, Value rhs, SourcePos pos);

    /**
     * @brief Subtract the given values.
     */
    Value sub(Value lhs, Value rhs, SourcePos pos);

    /**
     * @brief Multiply the given values.
     */
    Value mul(Value lhs, Value rhs, SourcePos pos);

    /**
     * @brief Divide the given values.
     */
    Value div(Value lhs, Value rhs, SourcePos pos);

    /**
     * @brief Compare the given values for equality.
     */
    Value equal(Value lhs, Value rhs, SourcePos pos);

    /**
     * @brief Compare the given values for inequality.
     */
    Value nequal(Value lhs, Value rhs, SourcePos pos);

    /**
     * @brief Less-than relation.
     */
    Value less(Value lhs, Value rhs, SourcePos pos);

    /**
     * @brief Less-than-or-equal relation.
     */
    Value lesseq(Value lhs, Value rhs, SourcePos pos);

    /**
     * @brief Greater-than relation.
     */
    Value greater(Value lhs, Value rhs, SourcePos pos);

    /**
     * @brief Greater-than-or-equal relation.
     */
    Value greatereq(Value lhs, Value rhs, SourcePos pos);

    /**
     * @brief Boolean AND.
     */
    Value bool_and(Value lhs, Value rhs, SourcePos pos);

    /**
     * @brief Boolean OR.
     */
    Value bool_or(Value lhs, Value rhs, SourcePos pos);

    /**
     * @brief Boolean NOT.
     */
    Value bool_not(Value val, SourcePos pos);

    /**
     * @brief Function call.
     */
    Value call(std::string func, std::vector<Value> &args, SourcePos pos);

    /**
     * @brief Create a variable with the given name and type.
     */
    Variable declare(const std::string &name, const Type &t);

    /**
     * @brief Assign the given value to the given variable.
     */
    void assign(const std::string &varname, Value val, SourcePos pos);

    /**
     * @brief Return the given value.
     */
    void return_(Value val, SourcePos pos);

    /** @} **/

    /**
     * @defgroup Control structures.
     *
     * Again, fairly thin abstractions over LLVM's primitives.
     *
     * @{
     */

    void create_function_prototype(Function f, std::string name);
    void create_and_start_function(Function f, std::vector<std::string> args,
                                   std::string name);

    void end_function(void);

    /** @} */

    /**
     * @defgroup Emitters.
     *
     * Finish the translation and output some sort of code.
     *
     * See the documentation of ModuleCodegen.
     *
     * @{
     */

    void validate(std::ostream &);
    void optimize(int opt_level);
    void emit_ir(std::ostream &fd);
    void emit_obj(int fd);
    void emit_asm(int fd);

    /** @} */

    /* This should just be a temporary shim until Translator abstracts away
     * this stuff. */
    
    llvm::IRBuilder<> &get_builder(void);
    Environment &get_env(void);
    llvm::LLVMContext &get_ctx(void);


private:
    std::unique_ptr<TranslatorImpl> pimpl;
};

}
