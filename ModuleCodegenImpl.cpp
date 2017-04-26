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

llvm::Type *TypeCodegen::codegen(const AST::Type &ty) {
    return boost::apply_visitor(*this, ty);
}

llvm::Type *TypeCodegen::operator()(const AST::IntType &it) {
    return llvm::IntegerType::get(context, it.nbits);
}

llvm::Type *TypeCodegen::operator()(const AST::UIntType &ut) {
    return llvm::IntegerType::get(context, ut.nbits);
}

llvm::Type *TypeCodegen::operator()(const AST::Float &_) {
    return llvm::Type::getFloatTy(context);
}

llvm::Type *TypeCodegen::operator()(const AST::Double &_) {
    return llvm::Type::getDoubleTy(context);
}

llvm::Type *TypeCodegen::operator()(const AST::Void &_) {
    return llvm::Type::getVoidTy(context);
}

llvm::Type *TypeCodegen::operator()(const AST::UserType &ut) {
    auto binding = env(ut.name);

    // Check that the binding is real.
    if (!binding.type) {
        // TODO: get source location for this error.
        throw Error("error", "no such type (\"" + ut.name + "\")",
                    fname, SourcePos(0, 0));
    }

    return env(ut.name).type;
}

llvm::Type *TypeCodegen::operator()(const std::unique_ptr<AST::Pointer> &ut) {
    return llvm::PointerType::get(codegen(ut->pointed), 0);
}

/*****************************************************************************
 * Code generation for L-Values.
 */

llvm::Value *LValueCodegen::codegen(const AST::LValue &val){
    return boost::apply_visitor(*this, val);
}

llvm::Value *LValueCodegen::operator()(const AST::Variable &var) {
    return env[var.name].inst;
}

llvm::Value *LValueCodegen::operator()(
        const std::unique_ptr<AST::Dereference> &deref) {
    /* If the dereference is an l-value, return just the referand. */
    return eg.codegen(deref->referand);
}

/*****************************************************************************
 * Code generation for expressions.
 */

llvm::Value *ExpressionCodegen::operator()(const AST::IntLiteral &lit) {
    auto *type = llvm::IntegerType::get(context, 64);
    return llvm::ConstantInt::get(type, lit.value, true);
}

llvm::Value *ExpressionCodegen::operator()(const AST::UIntLiteral &lit) {
    auto *type = llvm::IntegerType::get(context, 64);
    return llvm::ConstantInt::get(type, lit.value, false);
}

llvm::Value *ExpressionCodegen::operator()(const AST::FloatLiteral &lit) {
    return llvm::ConstantFP::get(context, llvm::APFloat(lit.value));
}

llvm::Value *ExpressionCodegen::operator()(
        const std::unique_ptr<AST::Dereference> &deref) {
    /* Get the address the dereference dereferences, */
    auto *addr = codegen(deref->referand);
    /* and load from it. */
    return builder.CreateLoad(addr);
}

llvm::Value *ExpressionCodegen::operator()(
        const std::unique_ptr<AST::Reference> &ref) {
    /* LValue codegenerators return addresses to the l-value. */
    LValueCodegen lc(context, builder, module, env, fname, *this);
    /* So just use one of those to codegen the referand. */
    return lc.codegen(ref->referand);
}

llvm::Value *ExpressionCodegen::operator()(const AST::Variable &var) {
    auto &binding = env[var.name];

    if (!binding.inst) {
        throw Error("error", "variable name (" + var.name + ") not found",
                    fname, var.pos);
    }

    return builder.CreateLoad(binding.inst, var.name.c_str());
}

/*****************************************************************************
 * Arithmetic expressions.
 */

llvm::Value *ExpressionCodegen::codegen(const AST::Expression &expr) {
    return boost::apply_visitor(*this, expr);
}

llvm::Value *ExpressionCodegen::operator()(
        const std::unique_ptr<AST::Binop> &binop) {
    auto *lhs = codegen(binop->lhs);
    auto *rhs = codegen(binop->rhs);
    auto *lhs_ty = lhs->getType();
    auto *rhs_ty = rhs->getType();

    // Case integer types.
    if (lhs_ty->isIntegerTy() && rhs_ty->isIntegerTy()) {
        unsigned lhs_bw = lhs_ty->getIntegerBitWidth();
        unsigned rhs_bw = rhs_ty->getIntegerBitWidth();
        if (lhs_bw < rhs_bw) {
            lhs = builder.CreateZExt(lhs, rhs_ty);
        } else if (rhs_bw < lhs_bw) {
            rhs = builder.CreateZExt(rhs, lhs_ty);
        }

        bool is_u1 = lhs_bw == 1;

        // TODO: add unsigned stuff.
        if (binop->op == "+") {
            return builder.CreateAdd(lhs, rhs);
        } else if (binop->op == "-") {
            return builder.CreateSub(lhs, rhs);
        } else if (binop->op == "*") {
            return builder.CreateMul(lhs, rhs);
        } else if (binop->op == "/") {
            return builder.CreateSDiv(lhs, rhs);
        } else if (binop->op == "^") {
            return builder.CreateXor(lhs, rhs);
        } else if (binop->op == "&") {
            return builder.CreateAnd(lhs, rhs);
        } else if (binop->op == "|") {
            return builder.CreateOr(lhs, rhs);
        } else if (binop->op == ">>") {
            return builder.CreateAShr(lhs, rhs);
        } else if (binop->op == "<<") {
            return builder.CreateShl(lhs, rhs);
        } else if (binop->op == "==") {
            return builder.CreateICmpEQ(lhs, rhs);
        } else if (binop->op == "!=") {
            return builder.CreateICmpNE(lhs, rhs);
        } else if (binop->op == "<") {
            return builder.CreateICmpSLT(lhs, rhs);
        } else if (binop->op == ">") {
            return builder.CreateICmpSGT(lhs, rhs);
        } else if (binop->op == "<=") {
            return builder.CreateICmpSLE(lhs, rhs);
        } else if (binop->op == ">=") {
            return builder.CreateICmpSGE(lhs, rhs);
        } else if (binop->op == "!=") {
            return builder.CreateICmpNE(lhs, rhs);
        } else if (binop->op == "&&") {
            if (!is_u1) {
                throw Error("error", "&& not permitted between integers of "
                                     "multiple bits", fname, binop->pos);
            }
            return builder.CreateAnd(lhs, rhs);
        } else if (binop->op == "||") {
            if (!is_u1) {
                throw Error("error", "|| not permitted between integers of "
                                     "multiple bits", fname, binop->pos);
            }
            return builder.CreateOr(lhs, rhs);
        } else {
            throw Error("error", "operation \"" + binop->op
                               + "\" not permitted between integers",
                        fname, binop->pos);
        }
    // Case float operation.
    } else if ((lhs_ty->isFloatTy() || lhs_ty->isDoubleTy())
            && (rhs_ty->isFloatTy() || rhs_ty->isDoubleTy())) {
        // Deal with width mismatches by extending the smaller.
        if (lhs_ty->isFloatTy() && rhs_ty->isDoubleTy()) {
            lhs = builder.CreateFPExt(lhs, rhs_ty);
        } else if (lhs_ty->isDoubleTy() && rhs_ty->isFloatTy()) {
            rhs = builder.CreateFPExt(rhs, lhs_ty);
        }

        if (binop->op == "+") {
            return builder.CreateFAdd(lhs, rhs);
        } else if (binop->op == "-") {
            return builder.CreateFSub(lhs, rhs);
        } else if (binop->op == "*") {
            return builder.CreateFMul(lhs, rhs);
        } else if (binop->op == "/") {
            return builder.CreateFDiv(lhs, rhs);
        // Can't interpret a float as bits.
        } else if (binop->op == "^" || binop->op == "&"
                || binop->op == "|" || binop->op == ">>"
                || binop->op == "<<") {
            throw Error("error", "bitwise operation not permitted between"
                                 "floating-point values", fname, binop->pos);
        }
    // Case pointer arithmetic.
    } else if (lhs_ty->isPointerTy() && rhs_ty->isIntegerTy()) {
        return builder.CreateGEP(lhs, rhs);
    }

    // TODO: support more types.

    throw Error("error", "binary operator not permitted between "
                         "expressions of different types",
                fname, binop->pos);
}

llvm::Value *ExpressionCodegen::operator()(
        const std::unique_ptr<AST::FunctionCall> &call) {

    std::vector<llvm::Value *>llvm_args;

    for (const auto &arg: call->args) {
        llvm_args.push_back(codegen(arg));
    }

    auto &fbinding = env[call->fname];

    if (!fbinding.get_type()->isFunctionTy()) {
        throw Error("error", "function \"" + call->fname + "\" not defined",
                    fname, call->pos);
    }

    if (!fbinding.inst) {
        throw Error("error", "function \"" + call->fname + "\" not defined",
                    fname, call->pos);
    }

    return builder.CreateCall(fbinding.inst, llvm_args);
}

llvm::Value *ExpressionCodegen::operator()(
        const std::unique_ptr<AST::Cast> &cast) {
    auto *dest_ty = TypeCodegen(context, builder, module, env, fname)
                        .codegen(*cast->t);
    auto *cast_instr = codegen(cast->arg);
    auto *source_ty = cast_instr->getType();

    if (source_ty == dest_ty) {
        return cast_instr;
    }

    // Integer truncation/extension.
    if (dest_ty->isIntegerTy() && source_ty->isIntegerTy()) {
        auto dest_bits = dest_ty->getIntegerBitWidth();
        auto src_bits = source_ty->getIntegerBitWidth();

        if (dest_bits < src_bits) {
            return builder.CreateTrunc(cast_instr, dest_ty);
        } else if (dest_bits > src_bits) {
            return builder.CreateSExt(cast_instr, dest_ty);
        }
    // Integer -> Float
    } else if (dest_ty->isIntegerTy() && source_ty->isFloatingPointTy()) {
        return builder.CreateFPToSI(cast_instr, dest_ty);
    // Float -> Integer
    } else if (dest_ty->isFloatingPointTy() && source_ty->isIntegerTy()) {
        return builder.CreateSIToFP(cast_instr, dest_ty);
    // Floating-point truncation.
    } else if (dest_ty->isFloatTy() && source_ty->isDoubleTy()) {
        return builder.CreateFPTrunc(cast_instr, dest_ty);
    // Floating-point extension.
    } else if (dest_ty->isDoubleTy() && source_ty->isFloatTy()) {
        return builder.CreateFPExt(cast_instr, dest_ty);
    } else if (dest_ty->isPointerTy() && source_ty->isPointerTy()) {
        return builder.CreateBitCast(cast_instr, dest_ty);
    // TODO: add more.
    }

    // TODO: add error-message representations for types.
    throw Error("type error", "could not perform cast", fname, cast->pos);
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
    LValueCodegen lc(context, builder, module, env, fname, expr_codegen);
    /* TODO: codegen for LValues. */
    auto *addr = lc.codegen(assignment->lhs);
    auto *rhs  = expr_codegen.codegen(assignment->rhs);

    builder.CreateStore(rhs, addr);
}

void StatementCodegen::operator()(const AST::Return &ret) {
    auto *res = expr_codegen.codegen(*ret.retval);
    if (res->getType() != ret_type) {
        throw Error("type error", "return expression does not match "
                                  "function's return type",
                                  fname, ret.pos);
    }

    builder.CreateRet(res);
}

void StatementCodegen::operator()(const AST::VoidReturn &ret) {
    if (!ret_type->isVoidTy()) {
        throw Error("type error", "cannot have void return in non-void"
                                  "function",
                                  fname, ret.pos);
    }

    builder.CreateRetVoid();
}

void StatementCodegen::operator()(
        const std::unique_ptr<AST::Declaration> &decl) {
    auto tg = TypeCodegen(context, builder, module, env, fname);
    auto *t = tg.codegen(decl->type);
    auto *alloca = builder.CreateAlloca(t, nullptr, decl->name.name);
    env[decl->name.name] = IdentBinding(alloca);
}

void StatementCodegen::operator()(
        const std::unique_ptr<AST::CompoundDeclaration> &cdecl) {
    auto tg = TypeCodegen(context, builder, module, env, fname);
    auto *t = tg.codegen(cdecl->type);
    auto *alloca = builder.CreateAlloca(t, nullptr, cdecl->name.name);
    env[cdecl->name.name] = IdentBinding(alloca);

    auto *val = expr_codegen.codegen(cdecl->rhs);
    if (val->getType() != t) {
        throw Error("type error", "type in compound declaration does not "
                                  "match declared type", fname, cdecl->pos);
    }

    builder.CreateStore(val, alloca);
}

void StatementCodegen::operator()(
        const std::unique_ptr<AST::IfStatement> &if_stmt) {
    /* Generate code for the condition. */
    auto *cond = expr_codegen.codegen(if_stmt->condition);

    llvm::Function *parent = builder.GetInsertBlock()->getParent();

    auto *then_bb = llvm::BasicBlock::Create(context, "then", parent);
    auto *else_bb = llvm::BasicBlock::Create(context, "else");
    auto *merge_bb = llvm::BasicBlock::Create(context, "merge");

    assert(then_bb);
    assert(else_bb);
    assert(merge_bb);

    builder.CreateCondBr(cond, then_bb, else_bb);

    // Generate "then" code.
    builder.SetInsertPoint(then_bb);
    // In a scope...
    env.push();
    for (const auto &arg: if_stmt->if_block) {
        codegen(arg);
    }
    // which we forget after exiting.
    env.pop();

    if (can_continue()) {
        builder.CreateBr(merge_bb);
    }

    then_bb = builder.GetInsertBlock();

    // Generate "else" code.
    parent->getBasicBlockList().push_back(else_bb);
    builder.SetInsertPoint(else_bb);
    env.push();
    for (const auto &arg: if_stmt->else_block) {
        codegen(arg);
    }
    env.pop();
    if (can_continue()) {
        builder.CreateBr(merge_bb);
    }

    // Start again at "merge".
    parent->getBasicBlockList().push_back(merge_bb);
    builder.SetInsertPoint(merge_bb);
}

void StatementCodegen::set_rettype(llvm::Type *ret) {
    ret_type = ret;
}

bool StatementCodegen::can_continue(void) {
    return builder.GetInsertBlock()->getTerminator() == nullptr;
}

/*****************************************************************************
 * @brief Top-level codegen.
 */

ModuleCodegenImpl::ModuleCodegenImpl(std::string name, std::string triple,
                                     std::string fname)
    : builder(context),
      module(llvm::make_unique<llvm::Module>(name, context)),
      stmt_cg(context, builder, *module, env, fname, NULL)
        {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string error;
    auto target_triple = llvm::Triple(triple);
    /* TODO: Allow target-specific information. */
    auto llvm_target =
        llvm::TargetRegistry::lookupTarget("", target_triple, error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.

    /* TODO: fix this to properly handle the error. */
    if (!llvm_target) {
        llvm::errs() << error;
    }

    /* TODO: give a more specific CPU. */
    auto cpu = "generic";
    auto features = "";
    llvm::TargetOptions options;
    auto reloc_model = llvm::Reloc::Model();

    target = llvm_target->createTargetMachine(triple, cpu, features,
                                              options, reloc_model);
    module->setDataLayout(target->createDataLayout());
    module->setTargetTriple(triple);
}

void ModuleCodegenImpl::codegen(const AST::TopLevel &tl) {
    boost::apply_visitor(*this, tl);
}

void ModuleCodegenImpl::emit_ir(std::ostream &out) {
   llvm::raw_os_ostream llvm_out(out);
   module->print(llvm_out, nullptr);
}

void ModuleCodegenImpl::emit_asm(int fd) {
    llvm::raw_fd_ostream llvm_out(fd, false);
    llvm::legacy::PassManager pass;
    auto ft = llvm::TargetMachine::CGFT_AssemblyFile;

    if (target->addPassesToEmitFile(pass, llvm_out, ft)) {
        llvm::errs() << "TargetMachine can't emit a file of this type";
    }

    pass.run(*module);
    llvm_out.flush();
}

void ModuleCodegenImpl::emit_obj(int fd) {
    llvm::raw_fd_ostream llvm_out(fd, false);
    llvm::legacy::PassManager pass;
    auto ft = llvm::TargetMachine::CGFT_ObjectFile;

    if (target->addPassesToEmitFile(pass, llvm_out, ft)) {
        llvm::errs() << "TargetMachine can't emit a file of this type";
    }

    pass.run(*module);
    llvm_out.flush();
}

void ModuleCodegenImpl::operator()(const AST::TypeDeclaration &td) {
    auto *t = llvm::StructType::create(context, td.name);
    env(td.name) = TypeBinding(t);
}

void ModuleCodegenImpl::operator()(const AST::StructDeclaration &sd) {
    // TODO
}

void ModuleCodegenImpl::operator()(const AST::FunctionDeclaration &fd) {
    std::vector<llvm::Type *> arg_types;
    TypeCodegen tg(context, builder, *module, env, fname);

    for (const auto &decl: fd.args) {
        arg_types.push_back(tg.codegen(decl->type));
    }

    auto *ret_type = tg.codegen(fd.ret_type);

    llvm::FunctionType *ft = llvm::FunctionType::get(ret_type, arg_types,
                                                     false);

    auto *result = llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                                          fd.name, module.get());

    env[fd.name] = IdentBinding(result);
}

void ModuleCodegenImpl::operator()(
        const std::unique_ptr<AST::FunctionDefinition> &fd) {
    std::vector<llvm::Type *> arg_types;
    TypeCodegen tg(context, builder, *module, env, fname);

    for (const auto &decl: fd->signature.args) {
        arg_types.push_back(tg.codegen(decl->type));
    }

    auto *ret_type = tg.codegen(fd->signature.ret_type);

    llvm::FunctionType *ft = llvm::FunctionType::get(ret_type, arg_types,
                                                     false);

    auto *result = llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                                          fd->signature.name, module.get());

    env[fd->signature.name] = IdentBinding(result);

    env.push();

    llvm::BasicBlock *bb = llvm::BasicBlock::Create(context, "entry", result);
    builder.SetInsertPoint(bb);

    unsigned i = 0;
    for (auto &arg: result->args()) {
        auto *ty = arg_types[i];
        auto arg_addr = builder.CreateAlloca(ty);
        builder.CreateStore(&arg, arg_addr);
        env[fd->signature.args[i++]->name.name] = IdentBinding(arg_addr);
    }

    stmt_cg.set_rettype(ret_type);

    for (const auto &arg: fd->block) {
        stmt_cg.codegen(arg);
    }

    stmt_cg.set_rettype(NULL);
    env.pop();
}

void ModuleCodegenImpl::optimize(int opt_level) {
    auto fpm = std::make_unique<llvm::legacy::PassManager>();
    if (opt_level >= 1) {
        // Iterated dominance frontier to convert most `alloca`s to SSA
        // register accesses.
        fpm->add(llvm::createPromoteMemoryToRegisterPass());
        fpm->add(llvm::createInstructionCombiningPass());
        // Reassociate expressions.
        fpm->add(llvm::createReassociatePass());
        // Eliminate common sub-expressions.
        fpm->add(llvm::createGVNPass());
        // Simplify the control flow graph.
        fpm->add(llvm::createCFGSimplificationPass());
        // Tail call elimination.
        fpm->add(llvm::createTailCallEliminationPass());
    }

    fpm->run(*module);
}

void ModuleCodegenImpl::validate(std::ostream &out) {
    llvm::raw_os_ostream ll_out(out);
    llvm::verifyModule(*module, &ll_out);
}

}
