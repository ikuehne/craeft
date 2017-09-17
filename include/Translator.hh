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

#include <memory>

#include "llvm/Support/Host.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

#include "Block.hh"
#include "Environment.hh"
#include "Value.hh"
#include "Type.hh"

namespace Craeft {

/**
 * @brief Abstract implementation of `IfThenElse`.
 */
struct IfThenElseImpl;

/**
 * @brief Abstract representation of a Craeft if/then/else structure.
 *
 * Should only be used through `Translator`'s methods on it.
 */
class IfThenElse {
public:
    IfThenElse(std::unique_ptr<IfThenElseImpl> pimpl);
    IfThenElse(IfThenElse &&other);
    ~IfThenElse(void);
    std::unique_ptr<IfThenElseImpl> pimpl;
};

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
     * @brief Access a field of the given struct.
     *
     * Return the actual value at that field.
     */
    Value field_access(Value lhs, std::string field, SourcePos pos);

    /**
     * @brief Get the address of the given field of the given struct pointer.
     *
     * @param ptr   A pointer to a struct type.
     * @param field A field of the pointed struct type.
     */
    Value field_address(Value ptr, std::string field, SourcePos pos);

    /**
     * @brief Function call.
     */
    Value call(std::string func, std::vector<Value> &args, SourcePos pos);

    /**
     * @brief Template function call.
     */
    Value call(std::string func, std::vector<Type> &templ_args,
               std::vector<Value> &v_args, SourcePos pos);

    /**
     * Get a string literal as a char pointer.
     */
    Value string_literal(const std::string &str);

    /**
     * @brief Create a variable with the given name and type.
     */
    Variable declare(const std::string &name, const Type &t);

    /**
     * @brief Assign the given value to the given variable.
     */
    void assign(const std::string &varname, Value val, SourcePos pos);

    /**
     * @brief Return the given value, or void if none provided.
     */
    void return_(Value val, SourcePos pos);
    void return_(SourcePos pos);

    /** @} **/

    /**
     * @defgroup Symbols.
     *
     * @{
     */

    /**
     * @brief Get the address of the given identifier on the stack.
     *
     * Raise an Error if not present.
     */
    Value get_identifier_addr(std::string ident, SourcePos pos);

    /**
     * @brief Get the value of the given identifier.
     *
     * Raise an Error if not present.
     */
    Value get_identifier_value(std::string ident, SourcePos pos);

    /**
     * @brief Look up the given type by name.
     */
    Type lookup_type(std::string tname, SourcePos pos);

    /**
     * @brief Push a new scope.
     */
    void push_scope(void);

    /**
     * @brief Pop the topmost scope.
     */
    void pop_scope(void);

    /**
     * @brief Bind the given name to the given type.
     */
    void bind_type(std::string, Type t);

    Type specialize_template(std::string template_name,
                             const std::vector<Type> &args,
                             SourcePos pos);

    /**
     * @brief Register a template function.
     */
    void register_template(std::string name,
                           std::shared_ptr<AST::FunctionDefinition>,
                           std::vector<std::string> args,
                           TemplateFunction func);

    /**
     * @brief Register a template struct.
     */
    void register_template(TemplateStruct, std::string name);

    Struct<TemplateType> respecialize_template(std::string template_name,
                                         const std::vector<TemplateType>
                                              &args,
                                         SourcePos pos);

    /** @} **/

    /**
     * @defgroup Control structures.
     *
     * @{
     */

    /**
     * @brief Create and return an IfThenElse structure.
     *
     * Opens a new namespace; new instructions are added in the "then" block.
     *
     * @param cond The condition of the "if".
     */
    IfThenElse create_ifthenelse(Value cond, SourcePos pos);

    /**
     * @brief Terminates the "then" and starts emitting instructions at the
     *        "else".
     */
    void point_to_else(IfThenElse &structure);

    /**
     * @brief Exit the if/then/else and start emitting instructions outside
     *        the structure.
     */
    void end_ifthenelse(IfThenElse structure);

    /** @} */

    /**
     * @defgroup Control structures.
     *
     * @{
     */

    void create_function_prototype(Function<> f, std::string name);

    void create_and_start_function(Function<> f,
                                   std::vector<std::string> args,
                                   std::string name);

    void create_struct(Struct<> t);
    void create_struct(TemplateStruct t);

    /**
     * @brief Point away from the current function.
     *
     * @return A vector of specializations referenced in the current function,
     *         which need to be defined in the current module or a linked
     *         module.
     */
    std::vector< std::pair< std::vector<Type>, TemplateValue> >
        end_function(void);

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

    /**
     * @brief Get the translator's LLVM context.
     */
    llvm::LLVMContext &get_ctx(void);

private:
    std::unique_ptr<TranslatorImpl> pimpl;
};

}
