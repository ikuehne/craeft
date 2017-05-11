/**
 * @file Block.hh
 *
 * @brief Representation of Craeft blocks and control structures.
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
#include <string>

#include "llvm/IR/IRBuilder.h"

#include "Value.hh"

// Forward declarations.
namespace llvm {
    class Function;
    class BasicBlock;
    class LLVMContext;
}

namespace Craeft {

/**
 * @brief Craeft blocks.
 *
 * Represent sequences of instructions to be performed in sequence.  Thinly
 * wrap LLVM's BasicBlocks.
 */
class Block {
public:
    /**
     * @brief Create a new Craeft block inside the given function.
     *
     * @param f The LLVM function with which to associate this Block.
     *
     * @param name The name of the block in the generated IR.
     */
    Block(llvm::Function *f, std::string name);

    /**
     * @brief Create a new Craeft block associated with no function.
     *
     * Note that the block will need to be associated with a function for its
     * code to end up in the resulting module.
     *
     * @param name The name of the block.
     */
    Block(llvm::LLVMContext &, std::string name);

    // Explicitly declare destructor because we have forward-declared pointer
    // members.
    ~Block(void);

    /**
     * @brief Associate this block with the given function.
     *
     * The block will be appended to the end of the function.
     */
    void associate(llvm::Function *f);

    /**
     * @brief Jump to the other block.
     *
     * This is a terminating instruction.
     */
    void jump_to(Block other);

    /**
     * @brief Add a conditional jump to the provided blocks.
     *
     * This is a terminating instruction.
     */
    void cond_jump(Value cond, Block then_b, Block else_b);

    /**
     * @brief Return with the given value, or `void` if none.
     *
     * This is a terminating instruction.
     */
    void return_(Value ret);
    void return_(void);

    /**
     * @brief Check if the block is already terminated.
     */
    bool is_terminated(void) { return terminated; }

    /**
     * @brief Get the underlying LLVM block.
     */
    llvm::BasicBlock *to_llvm(void) { return underlying; }

    /**
     * @brief Point the given IRBuilder at this block.
     */
    void point_builder(llvm::IRBuilder<> &);

    /**
     * @brief Get this block's LLVM parent function.
     */
    llvm::Function *get_parent(void) { return f; }

private:
    llvm::BasicBlock *underlying;
    llvm::Function *f;
    bool terminated;
};

}
