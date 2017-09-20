/**
 * @file ValueCodegen.cpp
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

#include "ValueCodegen.hh"
#include "TypeCodegen.hh"

namespace Craeft {

Value LValueCodegen::operator()(const AST::Variable &var) {
    return _translator.get_identifier_addr(var.name(), var.pos());
}

Value LValueCodegen::operator()(const AST::Dereference &deref) {
    /* If the dereference is an l-value, return just the referand. */
    return ValueCodegen(_translator).visit(deref.referand());
}

Value LValueCodegen::operator()(const AST::FieldAccess &fa) {
    if (auto *structure = llvm::dyn_cast<AST::LValue>(&fa.structure())) {
        return _translator.field_address(
                visit(*structure), fa.field(), fa.pos());
    }

    throw Error("parser error",
                "expected lvalue structure in lvalue access",
                fa.pos());
}

Value ValueCodegen::operator()(const AST::IntLiteral &lit) {
    SignedInt type(64);
    auto *llvm_type = type.to_llvm(_ctx);
    auto *llvm_int = llvm::ConstantInt::get(llvm_type, lit.value(), true);
    return Value(llvm_int, type);
}

Value ValueCodegen::operator()(const AST::UIntLiteral &lit) {
    UnsignedInt type(64);
    auto *llvm_type = type.to_llvm(_ctx);
    auto *llvm_int = llvm::ConstantInt::get(llvm_type, lit.value(), true) ;
    return Value(llvm_int, type);
}

Value ValueCodegen::operator()(const AST::FloatLiteral &lit) {
    Float type(DoublePrecision);
    llvm::APFloat constant(lit.value());
    return Value(llvm::ConstantFP::get(_ctx, constant), type);
}

Value ValueCodegen::operator()(const AST::StringLiteral &lit) {
    return _translator.string_literal(lit.value());
}

Value ValueCodegen::operator()(const AST::Dereference &deref) {
    return _translator.add_load(visit(deref.referand()), deref.pos());
}

Value ValueCodegen::operator()(const AST::FieldAccess &access) {
    auto lhs = visit(access.structure());

    return _translator.field_access(lhs, access.field(), access.pos());
}

Value ValueCodegen::operator()(const AST::Reference &ref) {
    /* LValue codegenerators return addresses to the l-value, so just use one
     * of those to codegen the referand. */
    return LValueCodegen(_translator).visit(ref.referand());
}

Value ValueCodegen::operator()(const AST::Variable &var) {
    return _translator.get_identifier_value(var.name(), var.pos());
}

Value ValueCodegen::operator()(const AST::Binop &binop) {
    auto lhs = visit(binop.lhs());
    auto rhs = visit(binop.rhs());

    auto pos = binop.pos();

    if (binop.op() == "<<") {
        return _translator.left_shift(lhs, rhs, pos);
    } else if (binop.op() == ">>") {
        return _translator.right_shift(lhs, rhs, pos);
    } else if (binop.op() == "&") {
        return _translator.bit_and(lhs, rhs, pos);
    } else if (binop.op() == "|") {
        return _translator.bit_or(lhs, rhs, pos);
    } else if (binop.op() == "^") {
        return _translator.bit_xor(lhs, rhs, pos);
    } else if (binop.op() == "+") {
        return _translator.add(lhs, rhs, pos);
    } else if (binop.op() == "-") {
        return _translator.sub(lhs, rhs, pos);
    } else if (binop.op() == "*") {
        return _translator.mul(lhs, rhs, pos);
    } else if (binop.op() == "/") {
        return _translator.div(lhs, rhs, pos);
    } else if (binop.op() == "==") {
        return _translator.equal(lhs, rhs, pos);
    } else if (binop.op() == "!=") {
        return _translator.nequal(lhs, rhs, pos);
    } else if (binop.op() == "<") {
        return _translator.less(lhs, rhs, pos);
    } else if (binop.op() == "<=") {
        return _translator.lesseq(lhs, rhs, pos);
    } else if (binop.op() == ">") {
        return _translator.greater(lhs, rhs, pos);
    } else if (binop.op() == ">=") {
        return _translator.greatereq(lhs, rhs, pos);
    } else if (binop.op() == "&&") {
        return _translator.bool_and(lhs, rhs, pos);
    } else if (binop.op() == "||") {
        return _translator.bool_or(lhs, rhs, pos);
    } else {
        throw Error("internal error", "unrecognized operator \"" + binop.op()
                                                                 + "\"", pos);
                    
    }
}

Value ValueCodegen::operator()(const AST::FunctionCall &call) {
    std::vector<Value> args;

    for (const auto &arg: call.args()) {
        args.push_back(visit(*arg));
    }

    return _translator.call(call.fname(), args, call.pos());
}

Value ValueCodegen::operator()(const AST::TemplateFunctionCall &call) {
    TypeCodegen tg(_translator);

    std::vector<Type> tmpl_args;

    for (const auto &arg: call.type_args()) {
        tmpl_args.push_back(tg.visit(*arg));
    }

    std::vector<Value> args;

    for (const auto &arg: call.value_args()) {
        args.push_back(visit(*arg));
    }

    return _translator.call(call.fname(), tmpl_args, args, call.pos());
}

Value ValueCodegen::operator()(const AST::Cast &cast) {
    auto dest_ty = TypeCodegen(_translator).visit(cast.type());

    auto cast_val = visit(cast.arg());

    return _translator.cast(cast_val, dest_ty, cast.pos());
}

}
