/**
 * @file ModuleCodegenImpl.cpp
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

#include <algorithm>
#include <iostream>

#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"

#include "ModuleCodegenImpl.hh"

namespace Craeft {

/*****************************************************************************
 * Code generation for types.
 */

Type TypeCodegen::codegen(const AST::Type &ty) {
    return boost::apply_visitor(*this, ty);
}

Type TypeCodegen::operator()(const AST::NamedType &it) {
    // TODO: Annotate type names with source positions.
    return translator.lookup_type(it.name, SourcePos(0, 0));
}

Type TypeCodegen::operator()(const AST::Void &_) {
    return Void();
}

Type TypeCodegen::operator()(const std::unique_ptr<AST::Pointer> &ut) {
    return Pointer<>(codegen(ut->pointed));
}

Type TypeCodegen::operator()(const std::unique_ptr<AST::TemplatedType> &ut) {
}

/*****************************************************************************
 * Code generation for L-Values.
 */

Value LValueCodegen::codegen(const AST::LValue &val){
    return boost::apply_visitor(*this, val);
}

Value LValueCodegen::operator()(const AST::Variable &var) {
    return translator.get_identifier_addr(var.name, var.pos);
}

Value LValueCodegen::operator()(
        const std::unique_ptr<AST::Dereference> &deref) {
    /* If the dereference is an l-value, return just the referand. */
    return eg.codegen(deref->referand);
}

Value LValueCodegen::operator()(const std::unique_ptr<AST::FieldAccess> &fa) {
    auto st = codegen(fa->structure);

    return translator.field_address(st, fa->field, fa->pos);
}

/*****************************************************************************
 * Code generation for expressions.
 */

Value ExpressionCodegen::operator()(const AST::IntLiteral &lit) {
    SignedInt type(64);
    auto &ctx = translator.get_ctx();
    return Value(llvm::ConstantInt::get(type.to_llvm(ctx), lit.value, true),
                 type);
}

Value ExpressionCodegen::operator()(const AST::UIntLiteral &lit) {
    UnsignedInt type(64);
    auto &ctx = translator.get_ctx();
    return Value(llvm::ConstantInt::get(type.to_llvm(ctx), lit.value, true),
                 type);
}

Value ExpressionCodegen::operator()(const AST::FloatLiteral &lit) {
    Float type(DoublePrecision);
    auto &ctx = translator.get_ctx();
    return Value(llvm::ConstantFP::get(ctx, llvm::APFloat(lit.value)),
                 type);
}

Value ExpressionCodegen::operator()(
        const std::unique_ptr<AST::Dereference> &deref) {
    return translator.add_load(codegen(deref->referand), deref->pos);
}

Value ExpressionCodegen::operator()(
        const std::unique_ptr<AST::Reference> &ref) {
    /* LValue codegenerators return addresses to the l-value. */
    LValueCodegen lc(translator, fname, *this);
    /* So just use one of those to codegen the referand. */
    return lc.codegen(ref->referand);
}

Value ExpressionCodegen::operator()(const AST::Variable &var) {
    return translator.get_identifier_value(var.name, var.pos);
}

/*****************************************************************************
 * Arithmetic expressions.
 */

Value ExpressionCodegen::codegen(const AST::Expression &expr) {
    return boost::apply_visitor(*this, expr);
}

Value ExpressionCodegen::operator()(
        const std::unique_ptr<AST::Binop> &binop) {
    // Special cases: for now, just struct field accesses.
    if (binop->op == ".") {
        auto lhs = codegen(binop->lhs);

        auto v = boost::get<AST::Variable>(&binop->rhs);

        if (!v) {
            throw Error("parse error", "expected name in struct field access",
                        fname, binop->pos);
        }

        return translator.field_access(lhs, v->name, binop->pos);
    } else if (binop->op == "->") {
        auto lhs = codegen(binop->lhs);

        auto v = boost::get<AST::Variable>(&binop->rhs);

        if (!v) {
            throw Error("parse error", "expected name in struct field access",
                        fname, binop->pos);
        }

        auto derefed = translator.add_load(lhs, binop->pos);

        return translator.field_access(derefed, v->name, binop->pos);
    }

    auto lhs = codegen(binop->lhs);
    auto rhs = codegen(binop->rhs);

    auto pos = binop->pos;

    if (binop->op == "<<") {
        return translator.left_shift(lhs, rhs, pos);
    } else if (binop->op == ">>") {
        return translator.right_shift(lhs, rhs, pos);
    } else if (binop->op == "&") {
        return translator.bit_and(lhs, rhs, pos);
    } else if (binop->op == "|") {
        return translator.bit_or(lhs, rhs, pos);
    } else if (binop->op == "^") {
        return translator.bit_xor(lhs, rhs, pos);
    } else if (binop->op == "+") {
        return translator.add(lhs, rhs, pos);
    } else if (binop->op == "-") {
        return translator.sub(lhs, rhs, pos);
    } else if (binop->op == "*") {
        return translator.mul(lhs, rhs, pos);
    } else if (binop->op == "/") {
        return translator.div(lhs, rhs, pos);
    } else if (binop->op == "==") {
        return translator.equal(lhs, rhs, pos);
    } else if (binop->op == "!=") {
        return translator.nequal(lhs, rhs, pos);
    } else if (binop->op == "<") {
        return translator.less(lhs, rhs, pos);
    } else if (binop->op == "<=") {
        return translator.lesseq(lhs, rhs, pos);
    } else if (binop->op == ">") {
        return translator.greater(lhs, rhs, pos);
    } else if (binop->op == ">=") {
        return translator.greatereq(lhs, rhs, pos);
    } else if (binop->op == "&&") {
        return translator.bool_and(lhs, rhs, pos);
    } else if (binop->op == "||") {
        return translator.bool_or(lhs, rhs, pos);
    } else {
        throw Error("internal error", "unrecognized operator \"" + binop->op
                                                                 + "\"",
                    fname, pos);
                    
    }
}

Value ExpressionCodegen::operator()(
        const std::unique_ptr<AST::FunctionCall> &call) {

    std::vector<Value> args;

    for (const auto &arg: call->args) {
        args.push_back(codegen(arg));
    }

    return translator.call(call->fname, args, call->pos);
}

Value ExpressionCodegen::operator()(
        const std::unique_ptr<AST::TemplateFunctionCall> &) {

}

Value ExpressionCodegen::operator()(
        const std::unique_ptr<AST::Cast> &cast) {
    auto dest_ty = TypeCodegen(translator, fname).codegen(*cast->t);

    auto cast_val = codegen(cast->arg);

    return translator.cast(cast_val, dest_ty, cast->pos);
}

/*****************************************************************************
 * Statement codegen.
 */

void StatementCodegen::codegen(const AST::Statement &stmt) {
    boost::apply_visitor(*this, stmt);
}

void StatementCodegen::operator()(const AST::Expression &expr) {
    expr_codegen.codegen(expr);
}

void StatementCodegen::operator()(
        const std::unique_ptr<AST::Assignment> &assignment) {

    LValueCodegen lc(translator, fname, expr_codegen);

    auto addr = lc.codegen(assignment->lhs);
    auto rhs  = expr_codegen.codegen(assignment->rhs);

    translator.add_store(addr, rhs, assignment->pos);
}

void StatementCodegen::operator()(const AST::Return &ret) {
    translator.return_(expr_codegen.codegen(*ret.retval), ret.pos);
}

void StatementCodegen::operator()(const AST::VoidReturn &ret) {
    translator.return_(ret.pos);
}

void StatementCodegen::operator()(
        const std::unique_ptr<AST::Declaration> &decl) {

    auto tg = TypeCodegen(translator, fname);
    auto t = tg.codegen(decl->type);
    translator.declare(decl->name.name, t);
}

void StatementCodegen::operator()(
        const std::unique_ptr<AST::CompoundDeclaration> &cdecl) {
    std::string name = cdecl->name.name;
    auto tg = TypeCodegen(translator, fname);
    auto t = tg.codegen(cdecl->type);
    Variable result = translator.declare(name, t);
    translator.add_store(result.get_val(),
                         expr_codegen.codegen(cdecl->rhs),
                         cdecl->pos);
}

void StatementCodegen::operator()(
        const std::unique_ptr<AST::IfStatement> &if_stmt) {

    /* Generate code for the condition. */
    auto cond = expr_codegen.codegen(if_stmt->condition);

    auto structure = translator.create_ifthenelse(cond, if_stmt->pos);

    for (const auto &arg: if_stmt->if_block) {
        codegen(arg);
    }

    translator.point_to_else(structure);

    // Generate "else" code.
    for (const auto &arg: if_stmt->else_block) {
        codegen(arg);
    }

    translator.end_ifthenelse(std::move(structure));
}

/*****************************************************************************
 * @brief Top-level codegen.
 */

ModuleCodegenImpl::ModuleCodegenImpl(std::string name, std::string triple,
                                     std::string fname)
    : translator(name, fname, triple), fname(fname),
      stmt_cg(translator, fname) {
}

void ModuleCodegenImpl::codegen(const AST::TopLevel &tl) {
    boost::apply_visitor(*this, tl);
}

void ModuleCodegenImpl::emit_ir(std::ostream &out) {
    translator.emit_ir(out);
}

void ModuleCodegenImpl::emit_asm(int fd) {
    translator.emit_asm(fd);
}

void ModuleCodegenImpl::emit_obj(int fd) {
    translator.emit_obj(fd);
}

void ModuleCodegenImpl::operator()(const AST::TypeDeclaration &td) {
    throw Error("error", "type declarations not implemented",
                fname, td.pos);
}

void ModuleCodegenImpl::operator()(const AST::StructDeclaration &sd) {
    std::vector<std::pair<std::string, std::shared_ptr<Type> > >fields;
    TypeCodegen tg(translator, fname);

    for (const auto &decl: sd.members) {
        auto t = std::make_shared<Type>(tg.codegen(decl->type));
        fields.push_back(std::pair<std::string, std::shared_ptr<Type> >
                                  (decl->name.name, t));
    }

    Struct<> t(fields, sd.name);

    translator.create_struct(t);
}

void ModuleCodegenImpl::operator()(const AST::TemplateStructDeclaration &) {

}

Function<> ModuleCodegenImpl::type_of_ast_decl(
        const AST::FunctionDeclaration &fd) {
    std::vector<std::shared_ptr<Type> > arg_types;
    TypeCodegen tg(translator, fname);

    for (const auto &decl: fd.args) {
        arg_types.push_back(std::make_shared<Type>
                                            (tg.codegen(decl->type)));
    }

    auto ret_type = std::make_shared<Type>
                                    (tg.codegen(fd.ret_type));

    return Function<>(ret_type, arg_types);
}

void ModuleCodegenImpl::operator()(const AST::FunctionDeclaration &fd) {
    auto ty = type_of_ast_decl(fd);
    translator.create_function_prototype(ty, fd.name);
}

void ModuleCodegenImpl::operator()(
        const std::unique_ptr<AST::FunctionDefinition> &fd) {
    auto ty = type_of_ast_decl(fd->signature);

    std::vector<std::string> arg_names;

    for (auto &decl: fd->signature.args) {
        arg_names.push_back(decl->name.name);
    }

    translator.create_and_start_function(ty, arg_names, fd->signature.name);

    for (const auto &arg: fd->block) {
        stmt_cg.codegen(arg);
    }

    translator.end_function();
}

void ModuleCodegenImpl::operator()(const AST::TemplateFunctionDefinition &) {

}

void ModuleCodegenImpl::optimize(int opt_level) {
    translator.optimize(opt_level);
}

void ModuleCodegenImpl::validate(std::ostream &out) {
    translator.validate(out);
}

}
