/**
 * @file Statements.hh
 *
 * @brief The AST nodes that represent statements.
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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "llvm/Support/Casting.h"

#include "AST/Expressions.hh"
#include "Error.hh"

namespace Craeft {

namespace AST {

/**
 * @brief ASTs for statements.
 *
 * Uses LLVM RTTI.
 */
class Statement {
public:
    enum StatementKind {
        ExpressionStatement,
        Return,
        VoidReturn,
        Assignment,
        Declaration,
        CompoundDeclaration,
        IfStatement
    };

    StatementKind kind(void) const {
        return _kind;
    }

    virtual ~Statement() {}
    SourcePos pos(void) const { return _pos; }

    void set_pos(SourcePos pos) { _pos = pos; }

    Statement(StatementKind kind, SourcePos pos): _kind(kind), _pos(pos) {}

private:
    StatementKind _kind;
    SourcePos _pos;
};

#define STATEMENT_CLASS(X)\
    static bool classof(const Statement *s) {\
        return s->kind() == StatementKind::X;\
    }\
    ~X(void) override {}

/**
 * @brief A statement consisting of an expression (e.g. `1 + 1;`).
 */
class ExpressionStatement: public Statement {
public:
    explicit ExpressionStatement(std::unique_ptr<Expression> expr)
        : Statement(StatementKind::ExpressionStatement, expr->pos()),
          _expr(std::move(expr)) {}

    const Expression &expr(void) const { return *_expr; }

    STATEMENT_CLASS(ExpressionStatement);
private:
    std::unique_ptr<Expression> _expr;
};

/**
 * @brief Return statement with a value (as opposed to a void return).
 */
class Return: public Statement {
public:
    Return(std::unique_ptr<Expression> retval, SourcePos pos)
        : Statement(StatementKind::Return, pos), _retval(std::move(retval)) {}

    const Expression &retval(void) const { return *_retval; }

    STATEMENT_CLASS(Return);
private:
    std::unique_ptr<Expression> _retval;
};

/**
 * @brief Void return statement (`return;`).
 */
class VoidReturn: public Statement {
public:
    VoidReturn(SourcePos pos): Statement(StatementKind::VoidReturn, pos) {}
    STATEMENT_CLASS(VoidReturn);
};

/**
 * @brief Variable declaration.
 */
class Declaration: public Statement {
public:
    Declaration(std::unique_ptr<Type> type,
                const AST::Variable &name,
                SourcePos pos)
        : Statement(StatementKind::Declaration, pos),
          _type(std::move(type)),
          _name(name) {}

    const Type &type(void) const { return *_type; }

    const Variable &name(void) const { return _name; }

    STATEMENT_CLASS(Declaration);
private:
    std::unique_ptr<Type> _type;
    Variable _name;
};

/**
 * @brief Assignments (with `=`).
 */
class Assignment: public Statement {
public:
    Assignment(std::unique_ptr<LValue> lhs,
               std::unique_ptr<Expression> rhs,
               SourcePos pos)
        : Statement(StatementKind::Assignment, pos),
          _lhs(std::move(lhs)),
          _rhs(std::move(rhs)) {}

    const LValue &lhs(void) const { return *_lhs; }
    const Expression &rhs(void) const { return *_rhs; }

    STATEMENT_CLASS(Assignment);
private:
    std::unique_ptr<LValue> _lhs;
    std::unique_ptr<Expression> _rhs;
};

/**
 * @brief A declaration combined with an assignment (`I32 x = 5;`).
 */
class CompoundDeclaration: public Statement {
public:
    CompoundDeclaration(std::unique_ptr<Type> type,
                        const Variable &name,
                        std::unique_ptr<Expression> rhs,
                        SourcePos pos)
        : Statement(StatementKind::CompoundDeclaration, pos),
          _type(std::move(type)),
          _name(name),
          _rhs(std::move(rhs)) {}

    const Type &type(void) const { return *_type; }
    const Variable &name(void) const { return _name; }
    const Expression &rhs(void) const { return *_rhs; }

    STATEMENT_CLASS(CompoundDeclaration);
private:
    std::unique_ptr<Type> _type;
    Variable _name;
    std::unique_ptr<Expression> _rhs;
};

/**
 * @brief An `if/else` block.
 */
class IfStatement: public Statement {
public:
    IfStatement(std::unique_ptr<Expression> condition,
                std::vector<std::unique_ptr<Statement>> if_block,
                std::vector<std::unique_ptr<Statement>> else_block,
                SourcePos pos)
        : Statement(StatementKind::IfStatement, pos),
          _condition(std::move(condition)),
          _if_block(std::move(if_block)),
          _else_block(std::move(else_block)) {}

    const Expression &condition(void) const { return *_condition; }

    const std::vector<std::unique_ptr<Statement>> &if_block(void) const {
        return _if_block;
    }

    const std::vector<std::unique_ptr<Statement>> &else_block(void) const {
        return _else_block;
    }

    STATEMENT_CLASS(IfStatement);
private:
    std::unique_ptr<Expression> _condition;
    std::vector<std::unique_ptr<Statement>> _if_block;
    std::vector<std::unique_ptr<Statement>> _else_block;

};

#undef STATEMENT_CLASS

/**
 * @brief Visitor for AST statements, parameterized over the return type.
 *
 * Derived classes should override `operator()`.
 */
template<typename Result>
class StatementVisitor {
public:
    virtual ~StatementVisitor() {}

    Result visit(const Statement &stmt) {
        switch (stmt.kind()) {
#define HANDLE(X) case Statement::StatementKind::X:\
                      return operator()(llvm::cast<X>(stmt));
            HANDLE(ExpressionStatement);
            HANDLE(Return);
            HANDLE(VoidReturn);
            HANDLE(Assignment);
            HANDLE(Declaration);
            HANDLE(CompoundDeclaration);
            HANDLE(IfStatement);
#undef HANDLE
        }
    }
private:
    virtual Result operator()(const ExpressionStatement &) = 0;
    virtual Result operator()(const Return &) = 0;
    virtual Result operator()(const VoidReturn &) = 0;
    virtual Result operator()(const Assignment &) = 0;
    virtual Result operator()(const Declaration &) = 0;
    virtual Result operator()(const CompoundDeclaration &) = 0;
    virtual Result operator()(const IfStatement &) = 0;
};

/**
 * @brief Pretty-print the given statement to the given stream.
 */
void print_statement(const Statement &stmt, std::ostream &out);

}
}
