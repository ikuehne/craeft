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

Type TypeCodegen::operator()(const AST::NamedType &it) {
    // TODO: Annotate type names with source positions.
    return translator.lookup_type(it.name(), SourcePos(0, 0));
}

Type TypeCodegen::operator()(const AST::Void &_) {
    return Void();
}

Type TypeCodegen::operator()(const AST::Pointer &ut) {
    return Pointer<>(visit(ut.pointed()));
}

Type TypeCodegen::operator()(const AST::TemplatedType &t) {
    std::vector<Type> args;

    for (const auto &arg: t.args()) {
        args.push_back(visit(*arg));
    }

    return translator.specialize_template(t.name(), args, SourcePos(0, 0));
}

/*****************************************************************************
 * Code generation for template types.
 */

TemplateType TemplateTypeCodegen::operator()(const AST::NamedType &it) {
    for (int i = 0; i < (int)args.size(); ++i) {
        if (it.name() == args[i]) {
            return i;
        }
    }
    
    return to_template(translator.lookup_type(it.name(), SourcePos(0, 0)));
}

TemplateType TemplateTypeCodegen::operator()(const AST::Void &_) {
    return Void();
}

TemplateType TemplateTypeCodegen::operator()(const AST::Pointer &ut) {
    return Pointer<TemplateType>(visit(ut.pointed()));
}

TemplateType TemplateTypeCodegen::operator()(const AST::TemplatedType &t) {
    std::vector<TemplateType> args;

    for (const auto &arg: t.args()) {
        args.push_back(visit(*arg));
    }

    return translator.respecialize_template(t.name(), args, SourcePos(0, 0));
}


/*****************************************************************************
 * Code generation for L-Values.
 */

Value LValueCodegen::operator()(const AST::Variable &var) {
    return translator.get_identifier_addr(var.name(), var.pos());
}

Value LValueCodegen::operator()(
        const AST::Dereference &deref) {
    /* If the dereference is an l-value, return just the referand. */
    return eg.visit(deref.referand());
}

Value LValueCodegen::operator()(const AST::FieldAccess &fa) {
    if (auto *structure = llvm::dyn_cast<AST::LValue>(&fa.structure())) {
        return translator.field_address(
                visit(*structure), fa.field(), fa.pos());
    }

    throw Error("parser error",
                "expected lvalue structure in lvalue access",
                fname,
                fa.pos());
}

/*****************************************************************************
 * Code generation for expressions.
 */

Value ExpressionCodegen::operator()(const AST::IntLiteral &lit) {
    SignedInt type(64);
    auto &ctx = translator.get_ctx();
    return Value(llvm::ConstantInt::get(type.to_llvm(ctx), lit.value(), true),
                 type);
}

Value ExpressionCodegen::operator()(const AST::UIntLiteral &lit) {
    UnsignedInt type(64);
    auto &ctx = translator.get_ctx();
    return Value(llvm::ConstantInt::get(type.to_llvm(ctx), lit.value(), true),
                 type);
}

Value ExpressionCodegen::operator()(const AST::FloatLiteral &lit) {
    Float type(DoublePrecision);
    auto &ctx = translator.get_ctx();
    return Value(llvm::ConstantFP::get(ctx, llvm::APFloat(lit.value())),
                 type);
}

Value ExpressionCodegen::operator()(const AST::StringLiteral &lit) {
    return translator.string_literal(lit.value());
}

Value ExpressionCodegen::operator()(const AST::Dereference &deref) {
    return translator.add_load(visit(deref.referand()), deref.pos());
}

Value ExpressionCodegen::operator()(const AST::FieldAccess &access) {
    auto lhs = visit(access.structure());

    return translator.field_access(lhs, access.field(), access.pos());
}

Value ExpressionCodegen::operator()(const AST::Reference &ref) {
    /* LValue codegenerators return addresses to the l-value. */
    LValueCodegen lc(translator, fname, *this);
    /* So just use one of those to codegen the referand. */
    return lc.visit(ref.referand());
}

Value ExpressionCodegen::operator()(const AST::Variable &var) {
    return translator.get_identifier_value(var.name(), var.pos());
}

/*****************************************************************************
 * Arithmetic expressions.
 */

Value ExpressionCodegen::operator()(const AST::Binop &binop) {
    auto lhs = visit(binop.lhs());
    auto rhs = visit(binop.rhs());

    auto pos = binop.pos();

    if (binop.op() == "<<") {
        return translator.left_shift(lhs, rhs, pos);
    } else if (binop.op() == ">>") {
        return translator.right_shift(lhs, rhs, pos);
    } else if (binop.op() == "&") {
        return translator.bit_and(lhs, rhs, pos);
    } else if (binop.op() == "|") {
        return translator.bit_or(lhs, rhs, pos);
    } else if (binop.op() == "^") {
        return translator.bit_xor(lhs, rhs, pos);
    } else if (binop.op() == "+") {
        return translator.add(lhs, rhs, pos);
    } else if (binop.op() == "-") {
        return translator.sub(lhs, rhs, pos);
    } else if (binop.op() == "*") {
        return translator.mul(lhs, rhs, pos);
    } else if (binop.op() == "/") {
        return translator.div(lhs, rhs, pos);
    } else if (binop.op() == "==") {
        return translator.equal(lhs, rhs, pos);
    } else if (binop.op() == "!=") {
        return translator.nequal(lhs, rhs, pos);
    } else if (binop.op() == "<") {
        return translator.less(lhs, rhs, pos);
    } else if (binop.op() == "<=") {
        return translator.lesseq(lhs, rhs, pos);
    } else if (binop.op() == ">") {
        return translator.greater(lhs, rhs, pos);
    } else if (binop.op() == ">=") {
        return translator.greatereq(lhs, rhs, pos);
    } else if (binop.op() == "&&") {
        return translator.bool_and(lhs, rhs, pos);
    } else if (binop.op() == "||") {
        return translator.bool_or(lhs, rhs, pos);
    } else {
        throw Error("internal error", "unrecognized operator \"" + binop.op()
                                                                 + "\"",
                    fname, pos);
                    
    }
}

Value ExpressionCodegen::operator()(const AST::FunctionCall &call) {
    std::vector<Value> args;

    for (const auto &arg: call.args()) {
        args.push_back(visit(*arg));
    }

    return translator.call(call.fname(), args, call.pos());
}

Value ExpressionCodegen::operator()(const AST::TemplateFunctionCall &call) {
    TypeCodegen tg(translator, fname);

    std::vector<Type> tmpl_args;

    for (const auto &arg: call.type_args()) {
        tmpl_args.push_back(tg.visit(*arg));
    }

    std::vector<Value> args;

    for (const auto &arg: call.value_args()) {
        args.push_back(visit(*arg));
    }

    return translator.call(call.fname(), tmpl_args, args, call.pos());
}

Value ExpressionCodegen::operator()(const AST::Cast &cast) {
    auto dest_ty = TypeCodegen(translator, fname).visit(cast.type());

    auto cast_val = visit(cast.arg());

    return translator.cast(cast_val, dest_ty, cast.pos());
}

/*****************************************************************************
 * Statement codegen.
 */

void StatementCodegen::operator()(const AST::ExpressionStatement &expr) {
    expr_codegen.visit(expr.expr());
}

void StatementCodegen::operator()(const AST::Assignment &assignment) {

    LValueCodegen lc(translator, fname, expr_codegen);

    auto addr = lc.visit(assignment.lhs());
    auto rhs  = expr_codegen.visit(assignment.rhs());

    translator.add_store(addr, rhs, assignment.pos());
}

void StatementCodegen::operator()(const AST::Return &ret) {
    translator.return_(expr_codegen.visit(ret.retval()), ret.pos());
}

void StatementCodegen::operator()(const AST::VoidReturn &ret) {
    translator.return_(ret.pos());
}

void StatementCodegen::operator()(const AST::Declaration &decl) {
    auto tg = TypeCodegen(translator, fname);
    auto t = tg.visit(decl.type());
    translator.declare(decl.name().name(), t);
}

void StatementCodegen::operator()(const AST::CompoundDeclaration &cdecl) {
    std::string name = cdecl.name().name();
    auto tg = TypeCodegen(translator, fname);
    auto t = tg.visit(cdecl.type());
    Variable result = translator.declare(name, t);
    translator.add_store(result.get_val(),
                         expr_codegen.visit(cdecl.rhs()),
                         cdecl.pos());
}

void StatementCodegen::operator()(const AST::IfStatement &if_stmt) {
    /* Generate code for the condition. */
    auto cond = expr_codegen.visit(if_stmt.condition());

    auto structure = translator.create_ifthenelse(cond, if_stmt.pos());

    for (const auto &arg: if_stmt.if_block()) {
        visit(*arg);
    }

    translator.point_to_else(structure);

    // Generate "else" code.
    for (const auto &arg: if_stmt.else_block()) {
        visit(*arg);
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
        auto t = std::make_shared<Type>(tg.visit(decl->type()));
        fields.push_back(std::pair<std::string, std::shared_ptr<Type> >
                                  (decl->name().name(), t));
    }

    Struct<> t(fields, sd.name);

    translator.create_struct(t);
}

void ModuleCodegenImpl::operator()(const AST::TemplateStructDeclaration &s) {
    std::vector<std::pair<std::string, std::shared_ptr<TemplateType> > >fields;
    TemplateTypeCodegen tg(translator, fname, s.argnames);

    for (const auto &decl: s.decl.members) {
        auto t = std::make_shared<TemplateType>(tg.visit(decl->type()));
        fields.push_back(std::pair<std::string,
                                  std::shared_ptr<TemplateType> >
                                  (decl->name().name(), t));
    }

    Struct<TemplateType> t(fields, s.decl.name);

    TemplateStruct tmpl(t, s.argnames.size());

    translator.register_template(tmpl, s.decl.name);
}

Function<> ModuleCodegenImpl::type_of_ast_decl(
        const AST::FunctionDeclaration &fd) {
    std::vector<std::shared_ptr<Type> > arg_types;
    TypeCodegen tg(translator, fname);

    for (const auto &decl: fd.args) {
        arg_types.push_back(std::make_shared<Type>(tg.visit(decl->type())));
    }

    auto ret_type = std::make_shared<Type>(tg.visit(*fd.ret_type));

    return Function<>(ret_type, arg_types);
}

void ModuleCodegenImpl::operator()(const AST::FunctionDeclaration &fd) {
    auto ty = type_of_ast_decl(fd);
    translator.create_function_prototype(ty, fd.name);
}

std::vector< std::pair< std::vector<Type>, TemplateValue> >
ModuleCodegenImpl::codegen_function_with_name(
        const AST::FunctionDefinition &fd,
        std::string name) {
    auto ty = type_of_ast_decl(fd.signature);

    std::vector<std::string> arg_names;

    for (auto &decl: fd.signature.args) {
        arg_names.push_back(decl->name().name());
    }

    translator.create_and_start_function(ty, arg_names, name);

    for (const auto &arg: fd.block) {
        stmt_cg.visit(*arg);
    }

    return translator.end_function();
}

void ModuleCodegenImpl::operator()(
        const std::unique_ptr<AST::FunctionDefinition> &fd) {
    auto specializations = codegen_function_with_name(*fd,
                                                      fd->signature.name);

    for (int i = 0; i < (int)specializations.size(); ++i) {
        const auto specialization = specializations[i];
        const auto &args = specialization.first;
        const auto &val = specialization.second;

        assert(val.arg_names.size() == args.size());

        translator.push_scope();

        for (int j = 0; j < (int)args.size(); ++j) {
            translator.bind_type(val.arg_names[j], args[j]);
        }

        auto name = mangle_name(val.fd->signature.name, args);

        // Add any specializations added in codegen for *this* specialization.
        auto new_specializations = codegen_function_with_name(*val.fd, name);

        for (const auto &specialization: new_specializations) {
            specializations.push_back(specialization);
        }

        translator.pop_scope();
    }
}

void ModuleCodegenImpl::operator()(const AST::TemplateFunctionDefinition &f) {
    auto name = f.def->signature.name;

    std::vector<std::shared_ptr<TemplateType> >arg_types;
    TemplateTypeCodegen tg(translator, fname, f.argnames);

    for (const auto &decl: f.def->signature.args) {
        arg_types.push_back(std::make_shared<TemplateType>
                                            (tg.visit(decl->type())));
    }

    auto ret_type = std::make_shared<TemplateType>
                                    (tg.visit(*f.def->signature.ret_type));

    auto t = Function<TemplateType>(ret_type, arg_types);

    translator.register_template(name, f.def, f.argnames,
                                 TemplateFunction(t, f.argnames));
}

void ModuleCodegenImpl::optimize(int opt_level) {
    translator.optimize(opt_level);
}

void ModuleCodegenImpl::validate(std::ostream &out) {
    translator.validate(out);
}

}
