/**
 * @file ParserImpl.cpp
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

#include <cstdlib>

#include <boost/type_index.hpp>

#include "ParserImpl.hh"
#include "VariantUtils.hh"

/*****************************************************************************
 * Utilities for converting tokens to simple AST nodes.
 */

namespace Craeft {

template <typename AstLiteral, typename TokLiteral>
static inline std::unique_ptr<AstLiteral> get_literal(
        TokLiteral tok, SourcePos pos) {
    return std::make_unique<AstLiteral>(tok.value, pos);
}

/*****************************************************************************
 * Utilities for checking tokens.
 */

bool is_arrow(const Tok::Token &tok) {
    auto op = llvm::dyn_cast<Tok::Operator>(&tok);

    if (!op) return false;

    return op->op == "->";
}

/*****************************************************************************
 * Utilities for transforming the AST.
 */

/**
 * @brief Visitor for verifying that expressions are not actually assignments.
 *
 * The parser cannot distinguish between the two at parse-time, so expressions
 * must be verified after the fact.
 */
class ExpressionVerifier: public AST::ExpressionVisitor<void> {
public:
    ExpressionVerifier(const std::string &fname): fname(fname) {}

private:
    /* By default, do nothing. */
    void operator()(const AST::IntLiteral &) override {}
    void operator()(const AST::UIntLiteral &) override {}
    void operator()(const AST::FloatLiteral &) override {}
    void operator()(const AST::StringLiteral &) override {}
    void operator()(const AST::Variable &) override {}
    void operator()(const AST::Reference &) override {}
    void operator()(const AST::Dereference &) override {}
    void operator()(const AST::FieldAccess &) override {}

    /* On a binop, check if it is an `=`.  `=` are returned by the expression
     * parser, but they are not actually part of an expression, so this
     * results in an error. */
    void operator()(const AST::Binop &op) override {
        if (op.op() == "=") {
            throw Error("parse error",
                        "\"=\" may not appear in an expression",
                        fname, op.pos());
        }

        visit(op.lhs());
        visit(op.rhs());
    }

    /* For other nodes, just visit their children. */
    void operator()(const AST::FunctionCall &fc) override {
        std::for_each(fc.args().begin(), fc.args().end(),
                      [this](const auto &arg) { visit(*arg); });
    }

    void operator()(const AST::TemplateFunctionCall &fc) override {
        std::for_each(fc.value_args().begin(), fc.value_args().end(),
                      [this](const auto &arg) { visit(*arg); });
    }


    void operator()(const AST::Cast &cast) override {
        visit(cast.arg());
    }

private:
    const std::string &fname;
};

inline void ParserImpl::verify_expression(const AST::Expression &expr) const {
    /* Just a thin wrapper over the ExpressionVerifier visitor. */
    ExpressionVerifier(fname).visit(expr);
}

inline std::unique_ptr<AST::LValue> ParserImpl::to_lvalue(
        std::unique_ptr<AST::Expression> expr, SourcePos pos) const {
    if (auto result = llvm::dyn_cast<AST::LValue>(expr.get())) {
        expr.release();
        result->set_pos(pos);
        return std::unique_ptr<AST::LValue>(result);
    }
   
    if (auto binop = llvm::dyn_cast<AST::Binop>(expr.get())) {
        if (binop->op() == ".") {
             if (auto *name = llvm::dyn_cast<AST::Variable>(&binop->rhs())) {
                 SourcePos lhs_pos = binop->lhs().pos();
                 auto lvalue = to_lvalue(binop->release_lhs(), lhs_pos);
                 return std::make_unique<AST::FieldAccess>(
                         std::move(lvalue), name->name(), binop->pos());
             } else { 
                 throw Error("parser error",
                             "expected field name in field access",
                             fname, pos);
             }
         } else if (binop->op() == "->") {
             if (auto *name = llvm::dyn_cast<AST::Variable>(&binop->rhs())) {
                 SourcePos lhs_pos = binop->lhs().pos();
                 auto lvalue = std::make_unique<AST::Dereference>
                                               (binop->release_lhs(),
                                                lhs_pos);
                 return std::make_unique<AST::FieldAccess>(
                         std::move(lvalue), name->name(), binop->pos());
             } else { 
                 throw Error("parser error",
                             "expected field name in field access",
                             fname, pos);
             }
         } 
    }

    throw Error("parser error", "expected l-value", fname, pos);
}

class AssignmentFactorizer
      : public AST::ConsumingExpressionVisitor<AST::Statement> {
public:
    AssignmentFactorizer(const ParserImpl *p): parser(p) {}
private:
    /* By default, do nothing. */
    AST::Statement operator()(std::unique_ptr<AST::IntLiteral> x) override {
        return std::move(x);
    }
    AST::Statement operator()(std::unique_ptr<AST::UIntLiteral> x) override {
        return std::move(x);
    }
    AST::Statement operator()(std::unique_ptr<AST::FloatLiteral>x ) override {
        return std::move(x);
    }
    AST::Statement operator()(std::unique_ptr<AST::StringLiteral> x) override {
        return std::move(x);
    }
    AST::Statement operator()(std::unique_ptr<AST::Variable> x) override {
        return std::move(x);
    }
    AST::Statement operator()(std::unique_ptr<AST::Reference> x) override {
        return std::move(x);
    }
    AST::Statement operator()(std::unique_ptr<AST::Dereference> x) override {
        return std::move(x);
    }
    AST::Statement operator()(std::unique_ptr<AST::FieldAccess> x) override {
        return std::move(x);
    }
    AST::Statement operator()(std::unique_ptr<AST::FunctionCall> x) override {
        return std::move(x);
    }
    AST::Statement operator()(
            std::unique_ptr<AST::TemplateFunctionCall> x) override {
        return std::move(x);
    }
    AST::Statement operator()(std::unique_ptr<AST::Cast> x) override {
        return std::move(x);
    }

    AST::Statement operator()(std::unique_ptr<AST::Binop> op) override {
        if (op->op() == "=") {
            parser->verify_expression(op->lhs());
            parser->verify_expression(op->rhs());
            return std::make_unique<AST::Assignment>(
                    parser->to_lvalue(op->release_lhs(), op->pos()),
                    op->release_rhs(), op->pos());
        }

        return std::move(op);
    }

private:
    const ParserImpl *parser;
};

inline AST::Statement ParserImpl::extract_assignments(
        std::unique_ptr<AST::Expression> expr) const {
    AssignmentFactorizer af(this);
    return af.visit(std::move(expr));
}

/*****************************************************************************
 * ParserImpl public methods.
 */

ParserImpl::ParserImpl(std::string fname): lexer(fname), fname(fname) {}

std::unique_ptr<AST::Expression> ParserImpl::parse_expression(void) {
    return parse_binop(0, parse_unary());
}

AST::Statement ParserImpl::parse_statement(void) {
    if (llvm::isa<Tok::TypeName>(lexer.get_tok())) {
        auto result = parse_declaration();
        find_and_shift(Tok::Semicolon(), "after declaration");
        return result;
    } else if (llvm::isa<Tok::Return>(lexer.get_tok())) {
        auto result = parse_return();
        find_and_shift(Tok::Semicolon(), "after return statement");
        return result;
    } else if (llvm::isa<Tok::If>(lexer.get_tok())) {
        return parse_if_statement();
    } else {
        auto result = parse_expression();
        find_and_shift(Tok::Semicolon(), "after top-level expression");
        return extract_assignments(std::move(result));
    }
}

AST::TopLevel ParserImpl::parse_toplevel(void) {
    if (llvm::isa<Tok::Fn>(lexer.get_tok())) {
        return parse_function();
    } else if (llvm::isa<Tok::Struct>(lexer.get_tok())) {
        return parse_struct_declaration();
    } else if (llvm::isa<Tok::Type>(lexer.get_tok())) {
        return parse_type_declaration();
    } else {
        _throw("expected function or type declaration at top level");
    }
}

bool ParserImpl::at_eof(void) const {
    return lexer.at_eof();
}

/*****************************************************************************
 * Parser methods for dealing with particular forms.
 */

std::vector<std::unique_ptr<AST::Expression>> ParserImpl::parse_expr_list(
        void) {
    std::vector<std::unique_ptr<AST::Expression>> exprs;

    bool cont;
    do {
        cont = false;
        exprs.push_back(parse_expression());

        if (llvm::isa<Tok::Comma>(lexer.get_tok())) {
            lexer.shift();
            cont = true;
        }
    } while(cont);

    return exprs;
}

std::vector<std::unique_ptr<AST::Type>> ParserImpl::parse_type_list(void) {
    std::vector<std::unique_ptr<AST::Type>> types;

    bool cont;
    do {
        cont = false;
        types.push_back(parse_type());

        if (llvm::isa<Tok::Comma>(lexer.get_tok())) {
            lexer.shift();
            cont = true;
        }
    } while(cont);

    return types;
}

std::unique_ptr<AST::Expression> ParserImpl::parse_variable(void) {
    auto tok = llvm::cast<Tok::Identifier>(lexer.get_tok());

    std::string id = tok.name;

    // Shift the name.
    lexer.shift();

    if (at_open_generic()) {
        // Shift the <:.
        lexer.shift();

        std::vector<std::unique_ptr<AST::Type>> t_args;

        if (!at_close_generic()) {
            t_args = parse_type_list();

            assert(t_args.size() > 0);
        }

        find_and_shift(Tok::Operator(":>"), "after template argument list");

        find_and_shift(Tok::OpenParen(), "in template function call");

        std::vector<std::unique_ptr<AST::Expression>> args;

        if (!llvm::isa<Tok::CloseParen>(lexer.get_tok())) {
            args = parse_expr_list();
        }

        // Shift the close paren.
        find_and_shift(Tok::CloseParen(), "after function argument list");

        return std::make_unique<AST::TemplateFunctionCall>
                (id, std::move(t_args), std::move(args), lexer.get_pos());
    }

    /* Case not function call. */
    if (!llvm::isa<Tok::OpenParen>(lexer.get_tok())) {
        return std::make_unique<AST::Variable>(id, lexer.get_pos());
    }

    // Shift the opening paren.
    lexer.shift();

    // Accumulate vector of args.
    std::vector<std::unique_ptr<AST::Expression>> args;

    if (!llvm::isa<Tok::CloseParen>(lexer.get_tok())) {
        args = parse_expr_list();
    }

    // Shift the close paren.
    find_and_shift(Tok::CloseParen(), "after function argument list");

    return std::make_unique<AST::FunctionCall>(
            std::move(id), std::move(args), lexer.get_pos());
}

std::unique_ptr<AST::Expression> ParserImpl::parse_unary(void) {
    auto start = lexer.get_pos();

    if (!llvm::isa<Tok::Operator>(lexer.get_tok())) {
        return parse_primary();
    }

    // Save and shift the operator.
    auto op = llvm::cast<Tok::Operator>(lexer.get_tok());
    lexer.shift();

    // Parse the operand.
    auto operand = parse_unary();

    if (op.op == "*") {
        return std::make_unique<AST::Dereference>(std::move(operand), start);
    } else if (op.op == "&") {
        return std::make_unique<AST::Reference>(
                to_lvalue(std::move(operand), start), start);
    }

    throw Error("parser error", "unrecognized operator \"" + op.op
                              + "\"", fname, start);
}

std::unique_ptr<AST::Expression> ParserImpl::parse_binop(
        int prec, std::unique_ptr<AST::Expression> lhs) {
    auto start = lexer.get_pos();

    while (true) {
        int old_prec = get_token_precedence();

        if (old_prec < prec) return lhs;

        // Thing in expression was not an operator.
        if (!llvm::isa<Tok::Operator>(lexer.get_tok())) {
            _throw("expected operator in arithmetic expression");
        }

        auto op = llvm::cast<Tok::Operator>(lexer.get_tok());

        lexer.shift();

        auto rhs = parse_unary();
        
        int new_prec = get_token_precedence();

        if (old_prec < new_prec) {
            rhs = parse_binop(old_prec + 1, std::move(rhs));
        }

        if (op.op == "." || op.op == "->") {
            if (auto *var = llvm::dyn_cast<AST::Variable>(rhs.get())) {

                if (op.op == "->") {
                    auto pos = lhs->pos();
                    lhs = std::make_unique<AST::Dereference>(
                            std::move(lhs), pos);
                }

                lhs = std::make_unique<AST::FieldAccess>(
                        std::move(lhs), var->name(), start);
                continue;
            }

            _throw("expected field name in struct access");
        }

        lhs = std::make_unique<AST::Binop>(
                op.op, std::move(lhs), std::move(rhs), start);
    }
}

std::unique_ptr<AST::Cast> ParserImpl::parse_cast(void) {

    auto start = lexer.get_pos();
    auto type = parse_type();

    // No closing parenthesis.
    find_and_shift(Tok::CloseParen(), "after type in cast");

    auto expr = parse_expression();

    return std::make_unique<AST::Cast>(std::move(type), std::move(expr),
                                       start);
}

std::unique_ptr<AST::Expression> ParserImpl::parse_parens(void) {
    auto start = lexer.get_pos();

    // Shift the opening paren.
    lexer.shift();

    // Might be a cast.
    if (llvm::isa<Tok::TypeName>(lexer.get_tok())) {
        auto cast = parse_cast();

        // Fix starting position of cast to opening paren.
        cast->set_pos(start);

        return std::move(cast);
    }

    auto contents = parse_expression();

    find_and_shift(Tok::CloseParen(), "in parenthesized expression");

    return contents;
}

std::unique_ptr<AST::Expression> ParserImpl::parse_primary(void) {
    const auto &tok = lexer.get_tok();
    switch (tok.kind()) {
        case Tok::Token::TokenKind::Identifier:
            return parse_variable();
        case Tok::Token::TokenKind::IntLiteral: {
            auto i_lit = llvm::cast<Tok::IntLiteral>(tok);
            auto result = get_literal<AST::IntLiteral>
                                     (i_lit, lexer.get_pos());
            lexer.shift();
            return std::move(result);
        }

        case Tok::Token::TokenKind::UIntLiteral: {
            auto u_lit = llvm::cast<Tok::UIntLiteral>(tok);
            auto result = get_literal<AST::UIntLiteral>
                                     (u_lit, lexer.get_pos());
            lexer.shift();
            return std::move(result);
        }

        case Tok::Token::TokenKind::FloatLiteral: {
            auto f_lit = llvm::cast<Tok::FloatLiteral>(tok);
            auto result = get_literal<AST::FloatLiteral>
                                     (f_lit, lexer.get_pos());
            lexer.shift();
            return std::move(result);
        }

        case Tok::Token::TokenKind::StringLiteral: {
            auto s_lit = llvm::cast<Tok::StringLiteral>(tok);
            auto result = get_literal<AST::StringLiteral>
                                     (s_lit, lexer.get_pos());
            lexer.shift();
            return std::move(result);
        }

        case Tok::Token::TokenKind::OpenParen: {
            return parse_parens();
        }

        default:
            _throw("expected expression");
    }
}

std::unique_ptr<AST::Type> ParserImpl::parse_type(void) {
    /* TODO: Handle parentheses, array types, etc. */
    auto tname = llvm::cast<Tok::TypeName>(lexer.get_tok()).name;

    // Shift off the typename.
    lexer.shift();

    std::unique_ptr<AST::Type> result
        = std::make_unique<AST::NamedType>(tname);

    if (at_open_generic()) {
        lexer.shift();

        std::vector<std::unique_ptr<AST::Type>> args;
        if (!at_close_generic()) {
            args = parse_type_list();
        }

        find_and_shift(Tok::Operator(":>"), "after template type");

        result = std::make_unique<AST::TemplatedType>(tname, std::move(args));
    }

    while (llvm::isa<Tok::Operator>(lexer.get_tok())) {
        auto op = llvm::cast<Tok::Operator>(lexer.get_tok());
        if (op.op != "*") break;

        result = std::make_unique<AST::Pointer>(std::move(result));
        lexer.shift();
    }

    return result;
}

AST::Statement ParserImpl::parse_declaration(void) {
    auto start = lexer.get_pos();

    if (!llvm::isa<Tok::TypeName>(lexer.get_tok())) {
        _throw("expected type name in declaration");
    }

    auto type = parse_type();

    if (!llvm::isa<Tok::Identifier>(lexer.get_tok())) {
        _throw("expected identifier in declaration.");
    }

    auto ident = llvm::cast<Tok::Identifier>(lexer.get_tok());

    auto var = AST::Variable(ident.name, lexer.get_pos());

    lexer.shift();

    if (llvm::isa<Tok::Semicolon>(lexer.get_tok())
     || llvm::isa<Tok::CloseParen>(lexer.get_tok())
     || llvm::isa<Tok::Comma>(lexer.get_tok())) {
        return std::make_unique<AST::Declaration>(std::move(type), var,
                                                  start);
    }

    if (!llvm::isa<Tok::Operator>(lexer.get_tok())) {
        _throw("expected equals sign in compound assignment");
    }

    auto op = llvm::cast<Tok::Operator>(lexer.get_tok());

    if (op.op != "=") {
        _throw("expected equals sign in compound assignment");
    }

    // Shift the equals sign.
    lexer.shift();

    auto rhs = parse_expression();

    return std::make_unique<AST::CompoundDeclaration>(std::move(type), var,
                                                      std::move(rhs), start);
}

std::unique_ptr<AST::IfStatement> ParserImpl::parse_if_statement(void) {
    auto start = lexer.get_pos();
    // Shift the "if".
    lexer.shift();

    auto cond = parse_expression();

    auto if_block = parse_block();

    std::vector<AST::Statement> else_block;

    if (!llvm::isa<Tok::Else>(lexer.get_tok())) {
        // No else block.
        return std::make_unique<AST::IfStatement>(std::move(cond),
                                                  std::move(if_block),
                                                  std::move(else_block),
                                                  start);
    }

    // Otherwise, shift the "else"
    lexer.shift();

    // and parse the corresponding block.
    else_block = parse_block();;

    return std::make_unique<AST::IfStatement>(std::move(cond),
                                              std::move(if_block),
                                              std::move(else_block),
                                              start);
}

AST::Statement ParserImpl::parse_return(void) {
    auto start = lexer.get_pos();
    // Shift the return.
    lexer.shift();

    if (llvm::isa<Tok::Semicolon>(lexer.get_tok())) {
        return AST::VoidReturn(start);
    }

    auto retval = parse_expression();

    return AST::Return(std::move(retval), start);
}

AST::TypeDeclaration ParserImpl::parse_type_declaration(void) {
    auto start = lexer.get_pos();
    // Shift the `type`.
    lexer.shift();

    const auto &tok = lexer.get_tok();
    auto *tname_ptr = llvm::dyn_cast<Tok::TypeName>(&tok);

    if (!tname_ptr) {
        _throw("expected type name in type declaration.");
    }

    auto tname = *tname_ptr;

    // Shift the type name.
    lexer.shift();

    return AST::TypeDeclaration(tname.name, start);
}

std::vector<std::unique_ptr<AST::Declaration> >
      ParserImpl::parse_declarations(void) {
    find_and_shift(Tok::OpenBrace(), "in declaration block");

    std::vector<std::unique_ptr<AST::Declaration> > result;

    // Until we get to the closing brace,
    while (!llvm::isa<Tok::CloseBrace>(lexer.get_tok())) {
        auto decl_tmp = parse_declaration();
        auto *decl = boost::get<std::unique_ptr<AST::Declaration>>(&decl_tmp);

        if (!decl) {
            _throw("expected variable declaration in struct definition");
        }

        if (!llvm::isa<Tok::Semicolon>(lexer.get_tok())) {
            _throw("expected semicolon after struct member declaration");
        }

        result.push_back(std::move(*decl));

        // Shift the semicolon.
        lexer.shift();
    }

    // Shift the closing brace.
    lexer.shift();

    return result;
}

AST::TopLevel ParserImpl::parse_struct_declaration(void) {
    auto start = lexer.get_pos();

    // Shift the `struct`.
    lexer.shift();

    if (at_open_generic()) {
        lexer.shift();

        const auto& tname_tok = lexer.get_tok();

        std::vector<std::string> type_list;

        if (!at_close_generic()) {
            bool cont;
            do {
                cont = false;
                auto *tname = llvm::dyn_cast<Tok::TypeName>(&tname_tok);

                if (!tname) {
                    _throw("expected type name in template argument list");
                }

                type_list.push_back(tname->name);

                lexer.shift();

                if (llvm::isa<Tok::Comma>(lexer.get_tok())) {
                    lexer.shift();
                    cont = true;
                }
            } while (cont);
        }

        find_and_shift(Tok::Operator(":>"), "after template argument list");

        const auto& struct_tok = lexer.get_tok();

        auto *tname_ptr = llvm::dyn_cast<Tok::TypeName>(&struct_tok);

        if (!tname_ptr) {
            _throw("expected type name in template struct declaration");
        }

        auto tname = *tname_ptr;

        // Shift the type name.
        lexer.shift();

        auto members = parse_declarations();

        return AST::TemplateStructDeclaration(tname.name,
                                              type_list,
                                              std::move(members),
                                              start);
    }

    const auto &tok = lexer.get_tok();
    auto *tname_ptr = llvm::dyn_cast<Tok::TypeName>(&tok);

    if (!tname_ptr) {
        _throw("expected type name in type declaration");
    }

    auto tname = *tname_ptr;

    // Shift the type name.
    lexer.shift();

    auto members = parse_declarations();

    return AST::StructDeclaration(tname.name, std::move(members), start);
}

AST::TopLevel ParserImpl::parse_function(void) {
    auto start = lexer.get_pos();

    bool templ = false;

    // Shift the `fn`.
    lexer.shift();

    std::vector<std::string> type_list;

    if (at_open_generic()) {
        templ = true;

        lexer.shift();

        if (!at_close_generic()) {
            bool cont;
            do {
                cont = false;
                const auto &tok = lexer.get_tok();
                auto *tname = llvm::dyn_cast<Tok::TypeName>(&tok);

                if (!tname) {
                    _throw("expected type name in function template "
                           "argument list");
                }

                type_list.push_back(tname->name);

                lexer.shift();

                if (llvm::isa<Tok::Comma>(lexer.get_tok())) {
                    lexer.shift();
                    cont = true;
                }
            } while (cont);
        }

        find_and_shift(Tok::Operator(":>"), "after template argument list");
    }

    const auto &tok = lexer.get_tok();

    auto *ident = llvm::dyn_cast<Tok::Identifier>(&tok);

    if (!ident) {
        _throw("expected identifier as function name");
    }

    auto fname = std::move(ident->name);

    // Shift the function name.
    lexer.shift();

    auto args = parse_arg_list();

    // Default to void type;
    std::unique_ptr<AST::Type> ret_type = std::make_unique<AST::Void>();

    // parse another return type if present.
    if (is_arrow(lexer.get_tok())) {
        lexer.shift();
        ret_type = parse_type();
    }

    AST::FunctionDeclaration decl(fname, std::move(args),
                                  std::move(ret_type), start);

    // If semicolon, this is just a forward declaration.
    if (llvm::isa<Tok::Semicolon>(lexer.get_tok())) {
        // Shift the semicolon.
        lexer.shift();
        return std::move(decl);
    }

    auto body = parse_block();

    if (templ) {
        return AST::TemplateFunctionDefinition(std::move(decl), type_list,
                                               std::move(body), start);

    }

    return std::make_unique<AST::FunctionDefinition>(std::move(decl),
                                                     std::move(body),
                                                     start);
}

std::vector<AST::Statement> ParserImpl::parse_block(void) {
    find_and_shift(Tok::OpenBrace(), "before block");

    std::vector<AST::Statement> result;

    while (!llvm::isa<Tok::CloseBrace>(lexer.get_tok())) {
        result.push_back(parse_statement());
    }

    lexer.shift();

    return result;
}

int ParserImpl::get_token_precedence(void) const {
    const auto &tok = lexer.get_tok();

    if (auto *op = llvm::dyn_cast<Tok::Operator>(&tok)) {
        auto i = precedences.find(op->op);
        if (i != precedences.end()) {
            return i->second;
        }
    }

    return -1;
}

std::vector<std::unique_ptr<AST::Declaration> >
      ParserImpl::parse_arg_list(void) {
    find_and_shift(Tok::OpenParen(), "before argument list");

    std::vector<std::unique_ptr<AST::Declaration> > args;

    while (!llvm::isa<Tok::CloseParen>(lexer.get_tok())) {
        auto decl_tmp = parse_declaration();
        auto *decl = boost::get<std::unique_ptr<AST::Declaration>>(&decl_tmp);

        if (!decl) {
            _throw("expected variable declaration in argument list");
        }

        args.push_back(std::move(*decl));

        if (llvm::isa<Tok::CloseParen>(lexer.get_tok())) break;
        find_and_shift(Tok::Comma(), "in function declaration");
    }

    // Shift the closing paren.
    lexer.shift();

    return args;
}

inline void ParserImpl::find_and_shift(const Tok::Token& expected,
                                       std::string at_place) {
    if (expected != lexer.get_tok()) {
        _throw("expected \"" + expected.repr() + "\" " + at_place);
    }

    lexer.shift();
}

inline bool ParserImpl::at_open_generic(void) {
    const auto &tok = lexer.get_tok();

    auto *_op = llvm::dyn_cast<Tok::Operator>(&tok);

    return _op && _op->op == "<:";
}

inline bool ParserImpl::at_close_generic(void) {
    const auto &tok = lexer.get_tok();

    auto *_op = llvm::dyn_cast<Tok::Operator>(&tok);

    return _op && _op->op == ":>";
}

[[noreturn]] inline void ParserImpl::_throw(std::string message) {
    throw Error("parser error", message, fname, lexer.get_pos());
}

}
