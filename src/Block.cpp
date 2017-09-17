/**
 * @file Block.cpp
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

#include "Block.hh"

namespace Craeft {

Block::Block(llvm::Function *f, std::string name)
    : underlying(llvm::BasicBlock::Create(f->getContext(), name, f)),
      f(f),
      terminated(false) {}

Block::~Block(void) {}

void Block::jump_to(Block other) {
    assert(!terminated);

    llvm::IRBuilder<> builder(underlying);

    builder.CreateBr(other.to_llvm());

    terminated = true;
}

void Block::cond_jump(Value cond, Block then_b, Block else_b) {
    assert(!terminated);

    llvm::IRBuilder<> builder(underlying);

    builder.CreateCondBr(cond.to_llvm(), then_b.to_llvm(), else_b.to_llvm());

    terminated = true;
}

void Block::return_(Value ret) {
    llvm::IRBuilder<> builder(underlying);

    builder.CreateRet(ret.to_llvm());

    terminated = true;
}

void Block::return_(void) {
    llvm::IRBuilder<> builder(underlying);

    builder.CreateRetVoid();

    terminated = true;
}

void Block::point_builder(llvm::IRBuilder<> &b) {
    b.SetInsertPoint(to_llvm());
}

}
