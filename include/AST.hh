/**
 * @file AST.hh
 *
 * @brief The classes comprising the abstract syntax tree.
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

#include <ostream>
#include <vector>

#include <boost/variant.hpp>

#include "AST/Expressions.hh"
#include "AST/Types.hh"
#include "Error.hh"

/* Architectural note: leaves of the AST are represented as small structs,
 * with a few public data members and a constructor taking corresponding
 * arguments.  Subtrees (for example, Expression) are boost::variants of those
 * other AST classes.
 *
 * Any of the AST classes which itself contains a potentially large AST class
 * should be held only as a std::unique_ptr, and AST classes that are not
 * known to be leaves should never be copied.
 */

namespace Craeft {

/**
 * @brief Contains all classes and utilities relating to the abstract syntax
 * tree.
 */
namespace AST {

/**
 * @defgroup Statements Classes for statements as represented in the AST.
 *
 * @{
 */

/**
 * @brief Return statements.
 */
struct Return {
    std::unique_ptr<Expression> retval;

    SourcePos pos;

    Return(std::unique_ptr<Expression> retval, SourcePos pos)
        : retval(std::move(retval)), pos(pos) {}
};

/**
 * @brief Empty return statements.
 */
struct VoidReturn {
    SourcePos pos;

    VoidReturn(SourcePos pos): pos(pos) {}
};

/**
 * @brief Variable declarations.
 */
struct Declaration {
    std::unique_ptr<Type> type;
    Variable name;

    SourcePos pos;

    Declaration(std::unique_ptr<Type> type, Variable name, SourcePos pos)
        : type(std::move(type)), name(name), pos(pos) {}
};

/**
 * @brief Assignments.
 */
struct Assignment {
    std::unique_ptr<LValue> lhs;
    std::unique_ptr<Expression> rhs;

    SourcePos pos;

    Assignment(std::unique_ptr<LValue> lhs,
               std::unique_ptr<Expression> rhs,
               SourcePos pos)
        : lhs(std::move(lhs)), rhs(std::move(rhs)), pos(pos) {}
};

/**
 * @brief Compound variable declarations.
 *
 * I.e. Typename varname = expression;
 */
struct CompoundDeclaration {
    std::unique_ptr<Type> type;
    Variable name;
    std::unique_ptr<Expression> rhs;

    SourcePos pos;

    CompoundDeclaration(std::unique_ptr<Type> type,
                        Variable name,
                        std::unique_ptr<Expression> rhs,
                        SourcePos pos)
        : type(std::move(type)), name(name),
          rhs(std::move(rhs)), pos(pos) {}
};

struct IfStatement;

typedef boost::variant< std::unique_ptr<Expression>,
                        Return,
                        VoidReturn,
                        std::unique_ptr<Assignment>,
                        std::unique_ptr<Declaration>,
                        std::unique_ptr<CompoundDeclaration>,
                        std::unique_ptr<IfStatement> >
    Statement;

struct IfStatement {
    std::unique_ptr<Expression> condition;
    std::vector<Statement> if_block;
    std::vector<Statement> else_block;

    SourcePos pos;

    IfStatement(std::unique_ptr<Expression> cond,
                std::vector<Statement> if_block,
                std::vector<Statement> else_block,
                SourcePos pos)
        : condition(std::move(cond)),
          if_block(std::move(if_block)),
          else_block(std::move(else_block)),
          pos(pos) {}
};

void print_statement(const Statement &, std::ostream &out);

/** @} */

/**
 * @defgroup Toplevel Forms that may be used at the source level.
 *
 * @{
 */

/**
 * @brief Type declaration.
 *
 * Forward declarations of types.
 */
struct TypeDeclaration {
    std::string name;

    SourcePos pos;

    TypeDeclaration(std::string name, SourcePos pos): name(name), pos(pos) {}
};

/**
 * @brief Struct declarations.
 */
struct StructDeclaration {
    std::string name;
    std::vector<std::unique_ptr<Declaration>> members;
    SourcePos pos;

    StructDeclaration(std::string name,
                      std::vector<std::unique_ptr<Declaration> > members,
                      SourcePos pos)
        : name(name), members(std::move(members)), pos(pos) {}
};

/**
 * @brief Template struct declarations.
 */
struct TemplateStructDeclaration {
    std::vector<std::string> argnames;
    
    StructDeclaration decl;

    TemplateStructDeclaration(
            std::string name,
            std::vector<std::string> argnames,
            std::vector<std::unique_ptr<Declaration> > members,
            SourcePos pos)
        : argnames(argnames),
          decl(name, std::move(members), pos) {}
};


/**
 * @brief Function declarations.
 */
struct FunctionDeclaration {
    std::string name;
    std::vector<std::unique_ptr<Declaration> > args;
    std::unique_ptr<Type> ret_type;

    SourcePos pos;

    FunctionDeclaration(std::string name,
                        std::vector<std::unique_ptr<Declaration> > args,
                        std::unique_ptr<Type> ret_type, SourcePos pos)
        : name(std::move(name)), args(std::move(args)),
          ret_type(std::move(ret_type)), pos(pos) {}
};

/**
 * @brief Function definitions.
 */
struct FunctionDefinition {
    FunctionDeclaration signature;
    std::vector<Statement> block;

    SourcePos pos;

    FunctionDefinition(FunctionDeclaration signature,
                       std::vector<Statement> block,
                       SourcePos pos)
        : signature(std::move(signature)), block(std::move(block)),
          pos(pos) {}
};

/**
 * @brief Template function definitions.
 */
struct TemplateFunctionDefinition {
    std::vector<std::string> argnames;
    std::shared_ptr<FunctionDefinition> def;

    TemplateFunctionDefinition(FunctionDeclaration signature,
                               std::vector<std::string> argnames,
                               std::vector<Statement> block,
                               SourcePos pos)
        : argnames(std::move(argnames)),
          def(new FunctionDefinition(std::move(signature),
                                     std::move(block), pos)) {}
};

typedef boost::variant< TypeDeclaration, StructDeclaration,
                        TemplateStructDeclaration,
                        FunctionDeclaration,
                        std::unique_ptr<FunctionDefinition>,
                        TemplateFunctionDefinition > TopLevel;

void print_toplevel(const TopLevel &, std::ostream &out);

/** @} */

}

}
