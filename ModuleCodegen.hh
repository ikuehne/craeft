/**
 * @file ModuleCodegen.hh
 *
 * @brief Codegen for single modules.
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

#include "AST.hh"

namespace Craeft {

class ModuleCodegenImpl;

/**
 * @brief Generating code for a module.
 */
class ModuleCodegen {
public:
    ModuleCodegen(std::string name, std::string filename,
                  std::string triple=llvm::sys::getDefaultTargetTriple());

    // You need explicitly declared destructors for PImpl classes...
    ~ModuleCodegen();

    /**
     * @brief Generate code for the given top-level AST node.
     */
    void codegen(const AST::TopLevel &);

    /**
     * @brief Emit LLVM IR to the given output stream.
     */
    void emit_ir(std::ostream &);

    /**
     * @brief Emit object code to the given output stream.
     *
     * @param fd A file descriptor to an open, writable file.  Will not be
     *           closed upon completion.
     */
    void emit_obj(int fd);

    /**
     * @brief Emit assembly code to the given output stream.
     *
     * @param fd A file descriptor to an open, writable file.  Will not be
     *           closed upon completion.
     */
    void emit_asm(int fd);

    /**
     * @brief Verify the generated module.
     *
     * @param out Stream to which to print errors.
     */
    void validate(std::ostream &out);

    /**
     * @brief Optimize the module.
     *
     * @param level The degree of optimization.  Currently 0 is no
     *              optimization, and >=1 is all optimizations.
     */
    void optimize(int level);

private:
    std::unique_ptr<ModuleCodegenImpl> pimpl;

};

}
