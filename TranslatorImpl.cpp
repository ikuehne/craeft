/**
 * @brief Actual implementation of the Translator.
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

#include "llvm/ADT/Triple.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"

#include "TranslatorImpl.hh"

using namespace std::placeholders;

namespace Craeft {

TranslatorImpl::TranslatorImpl(std::string module_name, std::string filename,
                               std::string triple)
    : fname(filename),
      builder(context),
      module(new llvm::Module(module_name, context)) {
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

enum LlvmCastType {
    SWidth,
    UWidth,
    SFloatToInt,
    UFloatToInt,
    SIntToFloat,
    UIntToFloat,
    FloatExt,
    FloatTrunc,
    PtrToInt,
    IntToPtr,
    PtrToPtr,
    Illegal
};

struct CastVisitor: public boost::static_visitor<LlvmCastType> {
    template<typename L, typename R>
    LlvmCastType operator()(const L &, const R &) const {
        return Illegal;
    }

    LlvmCastType operator()(const UnsignedInt &, const SignedInt &) const {
        return SWidth;
    }

    LlvmCastType operator()(const SignedInt &, const SignedInt &) const {
        return SWidth;
    }

    LlvmCastType operator()(const UnsignedInt &, const UnsignedInt &) const {
        return UWidth;
    }

    LlvmCastType operator()(const SignedInt &, const UnsignedInt &) const {
        return UWidth;
    }

    LlvmCastType operator()(const Float &, const SignedInt &) const {
        return SFloatToInt;
    }

    LlvmCastType operator()(const Float &, const UnsignedInt &) const {
        return UFloatToInt;
    }

    LlvmCastType operator()(const SignedInt &, const Float &) const {
        return SIntToFloat;
    }

    LlvmCastType operator()(const UnsignedInt &, const Float &) const {
        return UIntToFloat;
    }

    LlvmCastType operator()(const Float &l, const Float &r) const {
        if (l < r) {
            return FloatExt;
        } else {
            return FloatTrunc;
        }
    }

    LlvmCastType operator()(const Pointer &,
                            const SignedInt &) const {
        return PtrToInt;
    }

    LlvmCastType operator()(const std::unique_ptr<Pointer> &,
                            const UnsignedInt &) const {
        return PtrToInt;
    }

    LlvmCastType operator()(const SignedInt &,
                            const std::unique_ptr<Pointer> &) const {
        return IntToPtr;
    }

    LlvmCastType operator()(const UnsignedInt &,
                            const std::unique_ptr<Pointer> &) const {
        return IntToPtr;
    }

    LlvmCastType operator()(const std::unique_ptr<Pointer> &,
                            const std::unique_ptr<Pointer> &) const {
        return PtrToPtr;
    }
};

Value TranslatorImpl::cast(Value val, const Type &dest_ty, SourcePos pos) {
    Type source_ty = val.get_type();
    llvm::Value *inst;

    llvm::Type *dt = to_llvm_type(dest_ty);
    llvm::Value *v = val.to_llvm();

    if (source_ty == dest_ty) return val;

    switch (boost::apply_visitor(CastVisitor(), source_ty, dest_ty)) {
    case SWidth:
        inst = builder.CreateSExtOrTrunc(v, dt);
        break;
    case UWidth:
        inst = builder.CreateZExtOrTrunc(v, dt);
        break;
    case SFloatToInt:
        inst = builder.CreateFPToSI(v, dt);
        break;
    case UFloatToInt:
        inst = builder.CreateFPToUI(v, dt);
        break;
    case SIntToFloat:
        inst = builder.CreateSIToFP(v, dt);
        break;
    case UIntToFloat:
        inst = builder.CreateUIToFP(v, dt);
        break;
    case FloatExt:
        inst = builder.CreateFPExt(v, dt);
        break;
    case FloatTrunc:
        inst = builder.CreateFPTrunc(v, dt);
        break;
    case PtrToInt:
        inst = builder.CreatePtrToInt(v, dt);
    case IntToPtr:
        inst = builder.CreateIntToPtr(v, dt);
    case PtrToPtr:
        inst = builder.CreateBitCast(v, dt);
    case Illegal:
        // TODO: Add representations of types to make this more helpful.
        throw Error("type error", "cannot cast types", fname, pos);
    }

    return Value(inst, dest_ty);
}

Value TranslatorImpl::add_load(Value pointer, SourcePos pos) {
    auto ty = pointer.get_type();
    auto *pointer_ty = boost::get<Pointer>(&ty);
    // Make sure value is actually a pointer.
    if (!pointer_ty) {
        throw Error("type error", "cannot dereference non-pointer value",
                    fname, pos);
    }

    auto *pointed = pointer_ty->get_pointed();
    auto *inst = builder.CreateLoad(pointer.to_llvm());

    return Value(inst, *pointed);
}

Value TranslatorImpl::left_shift(Value val, Value nbits, SourcePos pos) {
    if (!nbits.is_integral()) {
        throw Error("type error", "cannot shift by non-integer value",
                    fname, pos);
    }

    if (!val.is_integral()) {
        throw Error("type error", "cannot shift non-integer value",
                    fname, pos);
    }

    auto *inst = builder.CreateShl(val.to_llvm(), nbits.to_llvm());
    return Value(inst, val.get_type());
}

Value TranslatorImpl::right_shift(Value val, Value nbits, SourcePos pos) {
    if (!nbits.is_integral()) {
        throw Error("type error", "cannot shift by non-integer value",
                    fname, pos);
    }

    llvm::Value *inst;

    if (is_type<SignedInt>(val.get_type())) {
        inst = builder.CreateAShr(val.to_llvm(), nbits.to_llvm());
    } else if (is_type<UnsignedInt>(val.get_type())) {
        inst = builder.CreateLShr(val.to_llvm(), nbits.to_llvm());
    } else {
        throw Error("type error", "cannot shift non-integer value",
                    fname, pos);
    }

    return Value(inst, val.get_type());
}

class Operator: public boost::static_visitor<Value> {
public:
    Operator(const Value &lhs, const Value &rhs,
             const SourcePos pos, const std::string &fname,
             llvm::IRBuilder<> &builder)
        : lhs(lhs), rhs(rhs), pos(pos), fname(fname), builder(builder) {}

    virtual ~Operator() = 0;

    virtual std::string get_op(void) const = 0;

    virtual Value sint_int_op(const Value &, const Value &) {
        type_error("cannot perform \"" + get_op() + "\" between signed "
                   "integers");
    }

    Value operator()(const SignedInt &, const SignedInt &) {
        return sint_int_op(lhs, rhs);
    }

    virtual Value uint_int_op(const Value &, const Value &) {
        type_error("cannot perform \"" + get_op() + "\" between unsigned "
                   "integers");
    }

    Value operator()(const UnsignedInt &, const UnsignedInt &) {
        return uint_int_op(lhs, rhs);
    }

    virtual Value float_float_op(const Value &, const Value &) {
        type_error("cannot perform \"" + get_op() + "\" between floats");
    }

    Value operator()(const Float &, const Float &) {
        return float_float_op(lhs, rhs);
    }

    virtual Value int_ptr_op(const Value &, const Value &) {
        type_error("cannot perform \"" + get_op() + "\" between an integer "
                   "and a pointer");
    }

    Value operator()(const UnsignedInt &, const Pointer &) {
        return int_ptr_op(lhs, rhs);
    }

    Value operator()(const SignedInt &, const Pointer &) {
        return int_ptr_op(lhs, rhs);
    }

    virtual Value ptr_int_op(const Value &, const Value &) {
        type_error("cannot perform \"" + get_op() + "\" between integers "
                   "and pointers");
    }

    Value operator()(const Pointer &, const UnsignedInt &) {
        return ptr_int_op(lhs, rhs);
    }

    Value operator()(const Pointer &, const SignedInt &) {
        return ptr_int_op(lhs, rhs);
    }

    virtual Value ptr_ptr_op(const Value &, const Value &) {
        type_error("cannot perform \"" + get_op() + "\" between pointers");
    }

    template<typename L, typename R>
    Value operator()(const L &, const R &) {
        type_error("illegal " + get_op());
    }

    Value apply(void) {
        return boost::apply_visitor(*this, lhs.get_type(), rhs.get_type());
    }

protected:
    llvm::IRBuilder<> &get_builder(void) const { return builder; }

    const SourcePos get_pos(void) const { return pos; }

    const std::string &get_fname(void) const { return fname; }

private:
    [[noreturn]] void type_error(std::string msg) const {
        throw Error("type error", msg, fname, pos);
    }

    const Value &lhs;
    const Value &rhs;
    const SourcePos pos;
    const std::string &fname;
    llvm::IRBuilder<> &builder;
};

/**
 * @brief Get the width of the provided integer type.
 */
static int get_width(const Type &integral) {
    auto *ty = to_llvm_type(integral);

    assert(ty->isIntegerTy());

    int nbits = ty->getIntegerBitWidth();

    return nbits;
}

/**
 * Return the wider of the two types.
 */
static const Type &get_wider(const Value &lhs, const Value &rhs) {
    auto &lty = lhs.get_type();
    auto &rty = rhs.get_type();

    assert(lty.is_integral());
    assert(rty.is_integral());

    int lhs_nbits = get_width(lty);
    int rhs_nbits = get_width(rty);

    if (lhs_nbits >= rhs_nbits) {
        return lty;
    }

    return rty;
}

class BitwiseOperator: public Operator {
public:
    BitwiseOperator(const Value &lhs, const Value &rhs,
                    const SourcePos pos, const std::string &fname,
                    llvm::IRBuilder<> &builder)
        : Operator(lhs, rhs, pos, fname, builder) {}

    virtual llvm::Value *perform(llvm::Value *, llvm::Value *)
        = 0;

    virtual Value sint_int_op(const Value &l, const Value &r) override {
        const auto &t = get_wider(l, r);
        llvm::Value *llvm_l;
        llvm::Value *llvm_r;
        if (t == l.get_type()) {
            llvm_l = get_builder().CreateSExtOrTrunc(l.to_llvm(),
                                                     to_llvm_type(t));
        } else {
            llvm_l = l.to_llvm();
        }

        if (t == r.get_type()) {
            llvm_r = get_builder().CreateSExtOrTrunc(r.to_llvm(),
                                                     to_llvm_type(t));
        } else {
            llvm_r = r.to_llvm();
        }

        auto *result = perform(llvm_l, llvm_r);
        return Value(result, t);
    }

    virtual Value uint_int_op(const Value &l, const Value &r) override {
        const auto &t = get_wider(l, r);
        llvm::Value *llvm_l;
        llvm::Value *llvm_r;
        if (t == l.get_type()) {
            llvm_l = get_builder().CreateZExtOrTrunc(l.to_llvm(),
                                                     to_llvm_type(t));
        } else {
            llvm_l = l.to_llvm();
        }

        if (t == r.get_type()) {
            llvm_r = get_builder().CreateZExtOrTrunc(r.to_llvm(),
                                                     to_llvm_type(t));
        } else {
            llvm_r = r.to_llvm();
        }

        auto *result = perform(llvm_l, llvm_r);
        return Value(result, t);
    }
};

#define make_bitwise(method, classname, op)\
    struct classname: public BitwiseOperator {\
        classname(const Value &lhs, const Value &rhs,\
                  const SourcePos pos, const std::string &fname,\
                  llvm::IRBuilder<> &builder)\
            : BitwiseOperator(lhs, rhs, pos, fname, builder) {}\
        virtual std::string get_op() const override { return (op); }\
        llvm::Value *perform(llvm::Value *l, llvm::Value *r) override {\
            return get_builder().method(l, r);\
        }\
    }

make_bitwise(CreateAnd, BitwiseAnd, "&");
make_bitwise(CreateOr, BitwiseOr, "|");
make_bitwise(CreateXor, BitwiseXor, "^");

Value TranslatorImpl::bit_and(Value lhs, Value rhs, SourcePos pos) {
    return BitwiseAnd(lhs, rhs, pos, fname, builder).apply();
}

Value TranslatorImpl::bit_or(Value lhs, Value rhs, SourcePos pos) {
    return BitwiseOr(lhs, rhs, pos, fname, builder).apply();
}

Value TranslatorImpl::bit_xor(Value lhs, Value rhs, SourcePos pos) {
    return BitwiseXor(lhs, rhs, pos, fname, builder).apply();
}

Value TranslatorImpl::bit_not(Value val, SourcePos pos) {
    if (!val.is_integral()) {
        throw Error("type error", "cannot perform bitwise operations on "
                                  "non-integral types", fname, pos);
    }

    auto *inst = builder.CreateNot(val.to_llvm());

    return Value(inst, val.get_type());
}

enum ArithmeticOpType {
    UIntIntOp,
    SIntIntOp,
    IntFloatOp,
    FloatIntOp,
    FloatFloatOp,
    PtrIntOp,
    IntPtrOp,
    PtrPtrOp,
    IllegalOp
};

struct OpVisitor: boost::static_visitor<ArithmeticOpType> {
    ArithmeticOpType operator()(const UnsignedInt &, const UnsignedInt &)
            const {
        return UIntIntOp;
    }

    ArithmeticOpType operator()(const SignedInt &, const SignedInt &) const {
        return SIntIntOp;
    }

    ArithmeticOpType operator()(const Float &, const SignedInt &) const {
        return FloatIntOp;
    }

    ArithmeticOpType operator()(const Float &, const UnsignedInt &) const {
        return FloatIntOp;
    }

    ArithmeticOpType operator()(const SignedInt &, const Float &) const {
        return IntFloatOp;
    }

    ArithmeticOpType operator()(const UnsignedInt &, const Float &) const {
        return IntFloatOp;
    }

    ArithmeticOpType operator()(const Float &, const Float &) const {
        return FloatFloatOp;
    }

    ArithmeticOpType operator()(const SignedInt &,
                                const std::unique_ptr<Pointer> &) const {
        return IntPtrOp;
    }

    ArithmeticOpType operator()(const UnsignedInt &,
                                const std::unique_ptr<Pointer> &) const {
        return IntPtrOp;
    }

    ArithmeticOpType operator()(const std::unique_ptr<Pointer> &,
                                const SignedInt &) const {
        return PtrIntOp;
    }

    ArithmeticOpType operator()(const std::unique_ptr<Pointer> &,
                                const UnsignedInt &) const {
        return PtrIntOp;
    }

    ArithmeticOpType operator()(const std::unique_ptr<Pointer> &,
                                const std::unique_ptr<Pointer> &) const {
        return PtrPtrOp;
    }

    template<typename L, typename R>
    ArithmeticOpType operator()(const L &, const R &) const {
        return IllegalOp;
    }
};

const Float &get_wider_type(const Type &l, const Type &r) {
    auto *l_ty = boost::get<Float>(&l);
    auto *r_ty = boost::get<Float>(&r);

    assert(l_ty);
    assert(r_ty);

    if (*l_ty < *r_ty) {
        return *r_ty;
    } else {
        return *l_ty;
    }
}

class AddOperator: public Operator {
public:
    AddOperator(const Value &lhs, const Value &rhs,
                const SourcePos pos, const std::string &fname,
                llvm::IRBuilder<> &builder)
        : Operator(lhs, rhs, pos, fname, builder) {}

    Value sint_int_op(const Value &l, const Value &r) override {
        auto *lty = to_llvm_type(l.get_type());
        auto *rty = to_llvm_type(r.get_type());
        int lbits = lty->getIntegerBitWidth();
        int rbits = rty->getIntegerBitWidth();

        if (lbits < rbits) {
            auto *l_instr = get_builder().CreateSExtOrTrunc(l.to_llvm(),
                                                            rty);
            auto *result = get_builder().CreateAdd(l_instr, r.to_llvm());
            return Value(result, r.get_type());
        } else {
            auto *r_instr = get_builder().CreateSExtOrTrunc(r.to_llvm(),
                                                            lty);
            auto *result = get_builder().CreateAdd(l.to_llvm(), r_instr);
            return Value(result, l.get_type());
        }
    }

    Value uint_int_op(const Value &l, const Value &r) override {
        auto *lty = to_llvm_type(l.get_type());
        auto *rty = to_llvm_type(r.get_type());
        int lbits = lty->getIntegerBitWidth();
        int rbits = rty->getIntegerBitWidth();

        if (lbits < rbits) {
            auto *l_instr = get_builder().CreateZExtOrTrunc(l.to_llvm(),
                                                            rty);
            auto *result = get_builder().CreateAdd(l_instr, r.to_llvm());
            return Value(result, r.get_type());
        } else {
            auto *r_instr = get_builder().CreateZExtOrTrunc(r.to_llvm(),
                                                            lty);
            auto *result = get_builder().CreateAdd(l.to_llvm(), r_instr);
            return Value(result, l.get_type());
        }
    }

    Value float_float_op(const Value &l, const Value &r) override {
        Type wider = get_wider_type(l.get_type(), r.get_type());

        llvm::Value *result;
        if (wider == l.get_type()) {
            auto *r_instr =
                get_builder().CreateFPCast(r.to_llvm(),
                                           to_llvm_type(wider));
            result = get_builder().CreateFAdd(l.to_llvm(), r_instr);
        } else {
            assert(wider == r.get_type());

            auto *l_instr =
                get_builder().CreateFPCast(l.to_llvm(),
                                           to_llvm_type(wider));
            result = get_builder().CreateFAdd(l_instr, r.to_llvm());
        }

        return Value(result, wider);
    }

    Value ptr_int_op(const Value &l, const Value &r) override {
        auto *result = get_builder().CreateGEP(l.to_llvm(), r.to_llvm());
        return Value(result, l.get_type());
    }

    Value int_ptr_op(const Value &l, const Value &r) override {
        return ptr_int_op(r, l);
    }

    std::string get_op(void) const override {
        return "+";
    }
};

Value TranslatorImpl::add(Value lhs, Value rhs, SourcePos pos) {
    return AddOperator(lhs, rhs, pos, fname, builder).apply();
}

class SubOperator: public Operator {
public:
    SubOperator(const Value &lhs, const Value &rhs,
                const SourcePos pos, const std::string &fname,
                llvm::IRBuilder<> &builder)
        : Operator(lhs, rhs, pos, fname, builder) {}

    Value sint_int_op(const Value &l, const Value &r) override {
        auto *lty = to_llvm_type(l.get_type());
        auto *rty = to_llvm_type(r.get_type());
        int lbits = lty->getIntegerBitWidth();
        int rbits = rty->getIntegerBitWidth();

        if (lbits < rbits) {
            auto *l_instr = get_builder().CreateSExtOrTrunc(l.to_llvm(),
                                                            rty);
            auto *result = get_builder().CreateSub(l_instr, r.to_llvm());
            return Value(result, r.get_type());
        } else {
            auto *r_instr = get_builder().CreateSExtOrTrunc(r.to_llvm(),
                                                            lty);
            auto *result = get_builder().CreateSub(l.to_llvm(), r_instr);
            return Value(result, l.get_type());
        }
    }

    Value uint_int_op(const Value &l, const Value &r) override {
        auto *lty = to_llvm_type(l.get_type());
        auto *rty = to_llvm_type(r.get_type());
        int lbits = lty->getIntegerBitWidth();
        int rbits = rty->getIntegerBitWidth();

        if (lbits < rbits) {
            auto *l_instr = get_builder().CreateZExtOrTrunc(l.to_llvm(),
                                                            rty);
            auto *result = get_builder().CreateSub(l_instr, r.to_llvm());
            return Value(result, r.get_type());
        } else {
            auto *r_instr = get_builder().CreateZExtOrTrunc(r.to_llvm(),
                                                            lty);
            auto *result = get_builder().CreateSub(l.to_llvm(), r_instr);
            return Value(result, l.get_type());
        }
    }

    Value float_float_op(const Value &l, const Value &r) override {
        Type wider = get_wider_type(l.get_type(), r.get_type());

        llvm::Value *result;
        if (wider == l.get_type()) {
            auto *r_instr =
                get_builder().CreateFPCast(r.to_llvm(),
                                           to_llvm_type(wider));
            result = get_builder().CreateFSub(l.to_llvm(), r_instr);
        } else {
            assert(wider == r.get_type());

            auto *l_instr =
                get_builder().CreateFPCast(l.to_llvm(),
                                           to_llvm_type(wider));
            result = get_builder().CreateFSub(l_instr, r.to_llvm());
        }

        return Value(result, wider);
    }

    Value ptr_int_op(const Value &l, const Value &r) override {
        // Construct negative `r` by subtracting it from 0.
        auto r_ty = r.get_type();
        auto *zero = llvm::ConstantInt::get(to_llvm_type(r_ty), 0);
        auto *negative = get_builder().CreateSub(zero, r.to_llvm());

        // Just do a regular GEP with a negative index.
        auto *result = get_builder().CreateGEP(l.to_llvm(), negative);
        return Value(result, l.get_type());
    }

    Value ptr_ptr_op(const Value &l, const Value &r) override {
        if (!(l.get_type() == r.get_type())) {
            throw Error("type error", "cannot subtract pointers of different "
                                      "types", get_fname(), get_pos());
        }
        auto *result = get_builder().CreatePtrDiff(l.to_llvm(), r.to_llvm());
        return Value(result, l.get_type());
    }

    std::string get_op(void) const override {
        return "-";
    }
};

Value TranslatorImpl::sub(Value lhs, Value rhs, SourcePos pos) {
    return SubOperator(lhs, rhs, pos, fname, builder).apply();
}

class MulOperator: public Operator {
public:
    MulOperator(const Value &lhs, const Value &rhs,
                const SourcePos pos, const std::string &fname,
                llvm::IRBuilder<> &builder)
        : Operator(lhs, rhs, pos, fname, builder) {}

    Value sint_int_op(const Value &l, const Value &r) override {
        auto *lty = to_llvm_type(l.get_type());
        auto *rty = to_llvm_type(r.get_type());
        int lbits = lty->getIntegerBitWidth();
        int rbits = rty->getIntegerBitWidth();

        if (lbits < rbits) {
            auto *l_instr = get_builder().CreateSExtOrTrunc(l.to_llvm(),
                                                            rty);
            auto *result = get_builder().CreateMul(l_instr, r.to_llvm());
            return Value(result, r.get_type());
        } else {
            auto *r_instr = get_builder().CreateSExtOrTrunc(r.to_llvm(),
                                                            lty);
            auto *result = get_builder().CreateMul(l.to_llvm(), r_instr);
            return Value(result, l.get_type());
        }
    }

    Value uint_int_op(const Value &l, const Value &r) override {
        auto *lty = to_llvm_type(l.get_type());
        auto *rty = to_llvm_type(r.get_type());
        int lbits = lty->getIntegerBitWidth();
        int rbits = rty->getIntegerBitWidth();

        if (lbits < rbits) {
            auto *l_instr = get_builder().CreateZExtOrTrunc(l.to_llvm(),
                                                            rty);
            auto *result = get_builder().CreateMul(l_instr, r.to_llvm());
            return Value(result, r.get_type());
        } else {
            auto *r_instr = get_builder().CreateZExtOrTrunc(r.to_llvm(),
                                                            lty);
            auto *result = get_builder().CreateMul(l.to_llvm(), r_instr);
            return Value(result, l.get_type());
        }
    }

    Value float_float_op(const Value &l, const Value &r) override {
        Type wider = get_wider_type(l.get_type(), r.get_type());

        llvm::Value *result;
        if (wider == l.get_type()) {
            auto *r_instr =
                get_builder().CreateFPCast(r.to_llvm(),
                                           to_llvm_type(wider));
            result = get_builder().CreateFMul(l.to_llvm(), r_instr);
        } else {
            assert(wider == r.get_type());

            auto *l_instr =
                get_builder().CreateFPCast(l.to_llvm(),
                                           to_llvm_type(wider));
            result = get_builder().CreateFMul(l_instr, r.to_llvm());
        }

        return Value(result, wider);
    }

    std::string get_op(void) const override {
        return "*";
    }
};

Value TranslatorImpl::mul(Value lhs, Value rhs, SourcePos pos) {
    return MulOperator(lhs, rhs, pos, fname, builder).apply();
}

class DivOperator: public Operator {
public:
    DivOperator(const Value &lhs, const Value &rhs,
                const SourcePos pos, const std::string &fname,
                llvm::IRBuilder<> &builder)
        : Operator(lhs, rhs, pos, fname, builder) {}

    Value sint_int_op(const Value &l, const Value &r) override {
        auto *lty = to_llvm_type(l.get_type());
        auto *rty = to_llvm_type(r.get_type());
        int lbits = lty->getIntegerBitWidth();
        int rbits = rty->getIntegerBitWidth();

        if (lbits < rbits) {
            auto *l_instr = get_builder().CreateSExtOrTrunc(l.to_llvm(),
                                                            rty);
            auto *result = get_builder().CreateSDiv(l_instr, r.to_llvm());
            return Value(result, r.get_type());
        } else {
            auto *r_instr = get_builder().CreateSExtOrTrunc(r.to_llvm(),
                                                            lty);
            auto *result = get_builder().CreateSDiv(l.to_llvm(), r_instr);
            return Value(result, l.get_type());
        }
    }

    Value uint_int_op(const Value &l, const Value &r) override {
        auto *lty = to_llvm_type(l.get_type());
        auto *rty = to_llvm_type(r.get_type());
        int lbits = lty->getIntegerBitWidth();
        int rbits = rty->getIntegerBitWidth();

        if (lbits < rbits) {
            auto *l_instr = get_builder().CreateZExtOrTrunc(l.to_llvm(),
                                                            rty);
            auto *result = get_builder().CreateUDiv(l_instr, r.to_llvm());
            return Value(result, r.get_type());
        } else {
            auto *r_instr = get_builder().CreateZExtOrTrunc(r.to_llvm(),
                                                            lty);
            auto *result = get_builder().CreateUDiv(l.to_llvm(), r_instr);
            return Value(result, l.get_type());
        }
    }

    Value float_float_op(const Value &l, const Value &r) override {
        Type wider = get_wider_type(l.get_type(), r.get_type());

        llvm::Value *result;
        if (wider == l.get_type()) {
            auto *r_instr =
                get_builder().CreateFPCast(r.to_llvm(),
                                           to_llvm_type(wider));
            result = get_builder().CreateFDiv(l.to_llvm(), r_instr);
        } else {
            assert(wider == r.get_type());

            auto *l_instr =
                get_builder().CreateFPCast(l.to_llvm(),
                                           to_llvm_type(wider));
            result = get_builder().CreateFDiv(l_instr, r.to_llvm());
        }

        return Value(result, wider);
    }

    std::string get_op(void) const override {
        return "/";
    }
};

Value TranslatorImpl::div(Value lhs, Value rhs, SourcePos pos) {
    return DivOperator(lhs, rhs, pos, fname, builder).apply();
}

Value TranslatorImpl::build_comp(Value lhs, Value rhs, SourcePos pos,
                                 llvm::CmpInst::Predicate sip,
                                 llvm::CmpInst::Predicate uip,
                                 llvm::CmpInst::Predicate fp) {
    OpVisitor v;

    auto lhs_ty = lhs.get_type();
    auto rhs_ty = rhs.get_type();

    Type wider(lhs_ty);
    llvm::Value *lhs_casted;
    llvm::Value *rhs_casted;
    llvm::Value *result;

    switch (boost::apply_visitor(v, lhs_ty, rhs_ty)) {
        case UIntIntOp:
            return apply_to_wider_integer(lhs, rhs, pos,
                                          [&](auto *lhs, auto *rhs) {
                return builder.CreateICmp(uip, lhs, rhs);
            });
        case SIntIntOp:
            return apply_to_wider_integer(lhs, rhs, pos,
                                          [&](auto *lhs, auto *rhs) {
                return builder.CreateICmp(sip, lhs, rhs);
            });
        case IntFloatOp:
        case FloatIntOp:
            throw Error("type error", "cannot compare integral and floating "
                                      "point values", fname, pos);
        case FloatFloatOp:
            wider = get_wider_type(lhs_ty, rhs_ty);
            lhs_casted = cast(lhs, wider, pos).to_llvm();
            rhs_casted = cast(rhs, wider, pos).to_llvm();

            result = builder.CreateFCmp(fp, lhs_casted, rhs_casted);
            return Value(result, wider);

        case PtrIntOp:
        case IntPtrOp:
            throw Error("type error", "cannot compare integers and pointers",
                        fname, pos);

        case PtrPtrOp:
            // TODO: Add != operator.
            if (!(lhs_ty == rhs_ty)) {
                throw Error("type error", "cannot compare pointers to "
                                          "different types", fname, pos);
            }

            result = builder.CreateICmp(uip, lhs.to_llvm(), rhs.to_llvm());
            return Value(result, lhs_ty);

        case IllegalOp:
            throw Error("type error", "illegal operation", fname, pos);
    }
}

Value TranslatorImpl::equal(Value lhs, Value rhs, SourcePos pos) {
    return build_comp(lhs, rhs, pos,
                      llvm::CmpInst::ICMP_EQ,
                      llvm::CmpInst::ICMP_EQ,
                      llvm::CmpInst::FCMP_OEQ);
}

Value TranslatorImpl::nequal(Value lhs, Value rhs, SourcePos pos) {
    return build_comp(lhs, rhs, pos,
                      llvm::CmpInst::ICMP_NE,
                      llvm::CmpInst::ICMP_NE,
                      llvm::CmpInst::FCMP_ONE);
}

Value TranslatorImpl::less(Value lhs, Value rhs, SourcePos pos) {
    return build_comp(lhs, rhs, pos,
                      llvm::CmpInst::ICMP_SLT,
                      llvm::CmpInst::ICMP_ULT,
                      llvm::CmpInst::FCMP_OLT);
}

Value TranslatorImpl::lesseq(Value lhs, Value rhs, SourcePos pos) {
    return build_comp(lhs, rhs, pos,
                      llvm::CmpInst::ICMP_SLE,
                      llvm::CmpInst::ICMP_ULE,
                      llvm::CmpInst::FCMP_OLE);
}

Value TranslatorImpl::greater(Value lhs, Value rhs, SourcePos pos) {
    return build_comp(lhs, rhs, pos,
                      llvm::CmpInst::ICMP_SGT,
                      llvm::CmpInst::ICMP_UGT,
                      llvm::CmpInst::FCMP_OGT);
}

Value TranslatorImpl::greatereq(Value lhs, Value rhs, SourcePos pos) {
    return build_comp(lhs, rhs, pos,
                      llvm::CmpInst::ICMP_SGE,
                      llvm::CmpInst::ICMP_UGE,
                      llvm::CmpInst::FCMP_OGE);
}

static inline bool is_u1(Value v) {
    auto ty = v.get_type();
    auto *lt = boost::get<UnsignedInt>(&ty);
    if (!lt) {
        return false;
    }

    int width = to_llvm_type(ty)->getIntegerBitWidth();

    return width == 1;
}

Value TranslatorImpl::bool_and(Value lhs, Value rhs, SourcePos pos) {
    if (!(is_u1(lhs) && is_u1(rhs))) {
        throw Error("type error", "logical operations only allowed between "
                                  "U1s", fname, pos);
    }

    auto *inst = builder.CreateAnd(lhs.to_llvm(), rhs.to_llvm());
    return Value(inst, lhs.get_type());
}

Value TranslatorImpl::bool_or(Value lhs, Value rhs, SourcePos pos) {
    if (!(is_u1(lhs) && is_u1(rhs))) {
        throw Error("type error", "logical operations only allowed between "
                                  "U1s", fname, pos);
    }

    auto *inst = builder.CreateOr(lhs.to_llvm(), rhs.to_llvm());
    return Value(inst, lhs.get_type());
}

Value TranslatorImpl::bool_not(Value val, SourcePos pos) {
    if (!is_u1(val)) {
        throw Error("type error", "logical not only allowed on U1s",
                    fname, pos);
    }

    auto *inst = builder.CreateNot(val.to_llvm());

    return Value(inst, val.get_type());
}


Value TranslatorImpl::call(std::string func, std::vector<Value> &args,
                           SourcePos pos) {
    std::vector<llvm::Value *>llvm_args;

    for (auto &arg: args) {
        llvm_args.push_back(arg.to_llvm());
    }

    if (!env.bound(func)) {
        throw Error("error", "function \"" + func + "\" not defined",
                    fname, pos);
    }

    auto fbinding = env.lookup_identifier(func, pos, fname);
    auto *ftype = boost::get<Function>(fbinding.get_type());

    if (!ftype) {
        throw Error("type error", "cannot call non-function value",
                    fname, pos);
    }

    for (unsigned i = 0; i < args.size(); ++i) {
        if (!(args[i].get_type() == *ftype->get_args()[i])) {
            throw Error("type error", "argument does not match function type",
                        fname, pos);
        }
    }

    auto *inst = builder.CreateCall(fbinding.get_val().to_llvm(), llvm_args);
    return Value(inst, *ftype->get_rettype());
}

Variable TranslatorImpl::declare(const std::string &varname, const Type &t) {
    auto *alloca = builder.CreateAlloca(to_llvm_type(t), nullptr, varname);
    return env.add_identifier(varname, Value(alloca, Pointer(t)));
}

void TranslatorImpl::assign(const std::string &varname, Value val,
                            SourcePos pos) {
    auto var = env.lookup_identifier(varname, pos, fname);

    if (!(val.get_type() == *var.get_type())) {
        throw Error("type error",
                    "cannot assign to variable of different type",
                    fname, pos);
    }

    // Get the address of the variable on the stack.
    auto *load = var.get_val().to_llvm();

    // Actually store the new value into that address.
    builder.CreateStore(val.to_llvm(), load);
}

void TranslatorImpl::create_function_prototype(Function f, std::string name) {
    auto *result = llvm::Function::Create(f.to_llvm(),
                                          llvm::Function::ExternalLinkage,
                                          name, module.get());

    env.add_identifier(name, Value(result, f));
}

void TranslatorImpl::create_and_start_function(Function f,
                                               std::string name) {
    auto *result = llvm::Function::Create(f.to_llvm(),
                                          llvm::Function::ExternalLinkage,
                                          name, module.get());
    env.add_identifier(name, Value(result, f));

    // Create the first block in the function.
    llvm::BasicBlock *bb = llvm::BasicBlock::Create(context, "entry", result);

    // Push a new namespace for the function.
    env.push();

    // TODO: keep track of return type for current function.
    builder.SetInsertPoint(bb);
}

void TranslatorImpl::end_function(void) {
    env.pop();

    // TODO: When return types are tracked, forget them here.
}

void TranslatorImpl::return_(Value val, SourcePos pos) {
    builder.CreateRet(val.to_llvm());
}

// TODO: Get rid of this once translator can replace builder.
llvm::IRBuilder<> &TranslatorImpl::get_builder(void) {
    return builder;
}

}
