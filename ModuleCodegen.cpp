/**
 * @file ModuleCodegen.cpp
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

#include "ModuleCodegen.hh"
#include "ModuleCodegenImpl.hh"

namespace Craeft {

ModuleCodegen::ModuleCodegen(std::string name, std::string filename,
                             std::string triple)
    : pimpl(new ModuleCodegenImpl(name, triple, filename)) {}

ModuleCodegen::~ModuleCodegen() {}

void ModuleCodegen::codegen(const AST::TopLevel &t) { pimpl->codegen(t); }

void ModuleCodegen::emit_ir(std::ostream &out) {
    pimpl->emit_ir(out);
}

void ModuleCodegen::emit_obj(int fd) {
    pimpl->emit_obj(fd);
}

void ModuleCodegen::emit_asm(int fd) {
    pimpl->emit_asm(fd);
}

void ModuleCodegen::validate(std::ostream &out) {
    pimpl->validate(out);
}

void ModuleCodegen::optimize(int level) {
    pimpl->optimize(level);
}

}
