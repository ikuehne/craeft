#include "Expression.hh"

namespace Compiler {

namespace AST {

struct ExpressionPrintVisitor: boost::static_visitor<void> {
    ExpressionPrintVisitor(std::ostream &out): out(out) {}

    template<typename T>
    void operator()(const T &lit) {
        out << typeid(T).name() << " {" << lit.value << "}";
    }

    void operator()(const Variable &var) {
        out << "Variable {" << var.name << "}";
    }

    void operator()(const std::unique_ptr<Binop> &bin) {
        out << "Binop {" << bin->op << ", ";
        boost::apply_visitor(*this, bin->lhs);
        out << ", ";
        boost::apply_visitor(*this, bin->rhs);
        out << "}";
    }

    void operator()(const std::unique_ptr<FunctionCall> &fc) {
        out << "FunctionCall {" << fc->fname;
        for (const auto &arg: fc->args) {
            out << ", ";
            boost::apply_visitor(*this, arg);
        }
        out << "}";
    }

    void operator()(const std::unique_ptr<Cast> &c) {
        /* TODO: fix type rendering to handle non-user types. */
        out << "Cast {" << boost::get<UserType>(*c->t).name << ", ";
        boost::apply_visitor(*this, c->arg);
        out << "}";
    }

    std::ostream &out;
};

void print_expr(const Expression &expr, std::ostream &out) {
    auto printer = ExpressionPrintVisitor(out);
    boost::apply_visitor(printer, expr);
}

}

}
