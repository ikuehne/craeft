#include "Translator.hh"

#include "TranslatorImpl.hh"

namespace Craeft {

Translator::Translator(std::string module_name, std::string filename,
                       std::string triple)
    : pimpl(new TranslatorImpl(module_name, filename, triple)) {}

Translator::~Translator() {}

Value Translator::cast(Value val, const Type &t, SourcePos pos) {
    return pimpl->cast(val, t, pos);
}

Value Translator::add_load(Value pointer, SourcePos pos) {
    return pimpl->add_load(pointer, pos);
}

void Translator::add_store(Value pointer, Value new_val, SourcePos pos) {
    pimpl->add_store(pointer, new_val, pos);
}

Value Translator::left_shift(Value val, Value nbits, SourcePos pos) {
    return pimpl->left_shift(val, nbits, pos);
}

Value Translator::right_shift(Value val, Value nbits, SourcePos pos) {
    return pimpl->right_shift(val, nbits, pos);
}

Value Translator::bit_and(Value lhs, Value rhs, SourcePos pos) {
    return pimpl->bit_and(lhs, rhs, pos);
}

Value Translator::bit_or(Value lhs, Value rhs, SourcePos pos) {
    return pimpl->bit_or(lhs, rhs, pos);
}

Value Translator::bit_xor(Value lhs, Value rhs, SourcePos pos) {
    return pimpl->bit_xor(lhs, rhs, pos);
}

Value Translator::bit_not(Value val, SourcePos pos) {
    return pimpl->bit_not(val, pos);
}

Value Translator::add(Value lhs, Value rhs, SourcePos pos) {
    return pimpl->add(lhs, rhs, pos);
}

Value Translator::sub(Value lhs, Value rhs, SourcePos pos) {
    return pimpl->sub(lhs, rhs, pos);
}

Value Translator::mul(Value lhs, Value rhs, SourcePos pos) {
    return pimpl->mul(lhs, rhs, pos);
}

Value Translator::div(Value lhs, Value rhs, SourcePos pos) {
    return pimpl->div(lhs, rhs, pos);
}

Value Translator::equal(Value lhs, Value rhs, SourcePos pos) {
    return pimpl->equal(lhs, rhs, pos);
}

Value Translator::nequal(Value lhs, Value rhs, SourcePos pos) {
    return pimpl->nequal(lhs, rhs, pos);
}

Value Translator::less(Value lhs, Value rhs, SourcePos pos) {
    return pimpl->less(lhs, rhs, pos);
}

Value Translator::lesseq(Value lhs, Value rhs, SourcePos pos) {
    return pimpl->lesseq(lhs, rhs, pos);
}

Value Translator::greater(Value lhs, Value rhs, SourcePos pos) {
    return pimpl->greater(lhs, rhs, pos);
}

Value Translator::greatereq(Value lhs, Value rhs, SourcePos pos) {
    return pimpl->greatereq(lhs, rhs, pos);
}

Value Translator::bool_and(Value lhs, Value rhs, SourcePos pos) {
    return pimpl->bool_and(lhs, rhs, pos);
}

Value Translator::bool_or(Value lhs, Value rhs, SourcePos pos) {
    return pimpl->bool_or(lhs, rhs, pos);
}

Value Translator::bool_not(Value val, SourcePos pos) {
    return pimpl->bool_not(val, pos);
}

Value Translator::field_access(Value lhs, std::string field, SourcePos pos) {
    return pimpl->field_access(lhs, field, pos);
}

Value Translator::field_address(Value ptr, std::string field, SourcePos pos) {
    return pimpl->field_address(ptr, field, pos);
}

Value Translator::call(std::string func, std::vector<Value> &args,
                       SourcePos pos) {
    return pimpl->call(func, args, pos);
}

Value Translator::call(std::string func, std::vector<Type> &templ_args,
                       std::vector<Value> &v_args, SourcePos pos) {
    return pimpl->call(func, templ_args, v_args, pos);
}

Variable Translator::declare(const std::string &name, const Type &t) {
    return pimpl->declare(name, t);
}

void Translator::assign(const std::string &varname, Value val,
                        SourcePos pos) {
    return pimpl->assign(varname, val, pos);
}

void Translator::return_(Value val, SourcePos pos) {
    return pimpl->return_(val, pos);
}

void Translator::return_(SourcePos pos) {
    return pimpl->return_(pos);
}

Value Translator::get_identifier_addr(std::string ident, SourcePos pos) {
    return pimpl->get_identifier_addr(ident, pos);
}
Value Translator::get_identifier_value(std::string ident, SourcePos pos) {
    return pimpl->get_identifier_value(ident, pos);
}
Type Translator::lookup_type(std::string tname, SourcePos pos) {
    return pimpl->lookup_type(tname, pos);
}
void Translator::push_scope(void) {
    pimpl->push_scope();
}
void Translator::pop_scope(void) {
    pimpl->pop_scope();
}

void Translator::bind_type(std::string name, Type t) {
    pimpl->bind_type(name, t);
}

Type Translator::specialize_template(std::string template_name,
                                     const std::vector<Type> &args,
                                     SourcePos pos) {
    return pimpl->specialize_template(template_name, args, pos);
}
void Translator::register_template(std::string name,
                                   std::shared_ptr<AST::FunctionDefinition> d,
                                   std::vector<std::string> args,
                                   TemplateFunction func) {
    pimpl->register_template(name, d, args, func);
}
void Translator::register_template(TemplateStruct s, std::string name) {
    pimpl->register_template(s, name);
}
Struct<TemplateType> Translator::respecialize_template(
        std::string template_name, const std::vector<TemplateType> &args,
        SourcePos pos) {
    return pimpl->respecialize_template(template_name, args, pos);
}
IfThenElse Translator::create_ifthenelse(Value cond, SourcePos pos) {
    return pimpl->create_ifthenelse(cond, pos);
}
void Translator::point_to_else(IfThenElse &structure) {
    pimpl->point_to_else(structure);
}

void Translator::end_ifthenelse(IfThenElse structure) {
    pimpl->end_ifthenelse(std::move(structure));
}

void Translator::create_function_prototype(Function<> f, std::string name) {
    pimpl->create_function_prototype(f, name);
}
void Translator::create_and_start_function(Function<> f,
                                           std::vector<std::string> args,
                                           std::string name) {
    pimpl->create_and_start_function(f, args, name);
}

void Translator::create_struct(Struct<> t) {
    pimpl->create_struct(t);
}

std::vector< std::pair< std::vector<Type>, TemplateValue> >
Translator::end_function(void) {
    return pimpl->end_function();
}

void Translator::validate(std::ostream &out) {
    pimpl->validate(out);
}
void Translator::optimize(int opt_level) {
    pimpl->optimize(opt_level);
}
void Translator::emit_ir(std::ostream &fd) {
    pimpl->emit_ir(fd);
}
void Translator::emit_obj(int fd) {
    pimpl->emit_obj(fd);
}
void Translator::emit_asm(int fd) {
    pimpl->emit_asm(fd);
}

llvm::LLVMContext &Translator::get_ctx(void) {
    return pimpl->get_ctx();
}

}
