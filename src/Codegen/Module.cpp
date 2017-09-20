/**
 * @file Codegen/Module.cpp
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

#include "Codegen/Module.hh"
#include "Codegen/ModuleImpl.hh"

namespace Craeft {

namespace Codegen {

ModuleGen::ModuleGen(std::string name, std::string filename,
                             std::string triple)
    : pimpl(new ModuleGenImpl(name, triple, filename)) {}

ModuleGen::~ModuleGen() {}

void ModuleGen::codegen(const AST::Toplevel &t) { pimpl->visit(t); }

void ModuleGen::emit_ir(std::ostream &out) {
    pimpl->emit_ir(out);
}

void ModuleGen::emit_obj(int fd) {
    pimpl->emit_obj(fd);
}

void ModuleGen::emit_asm(int fd) {
    pimpl->emit_asm(fd);
}

void ModuleGen::validate(std::ostream &out) {
    pimpl->validate(out);
}

void ModuleGen::optimize(int level) {
    pimpl->optimize(level);
}

}
}
