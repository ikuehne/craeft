/**
 * @file Toplevel.hh
 *
 * @brief The AST nodes that represent top-level forms.
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

#include "AST/Statements.hh"

namespace Craeft {

namespace AST {

class Toplevel {
public:
    enum ToplevelKind {
        TypeDeclaration,
        StructDeclaration,
        TemplateStructDeclaration,
        FunctionDeclaration,
        FunctionDefinition,
        TemplateFunctionDefinition
    };

    ToplevelKind kind(void) const {
        return _kind;
    }

    virtual ~Toplevel() {}
    SourcePos pos(void) const { return _pos; }

    void set_pos(SourcePos pos) { _pos = pos; }

    Toplevel(ToplevelKind kind, SourcePos pos): _kind(kind), _pos(pos) {}

private:
    ToplevelKind _kind;
    SourcePos _pos;
};

#define TOPLEVEL_CLASS(X)\
    static bool classof(const Toplevel *t) {\
        return t->kind() == ToplevelKind::X;\
    }\
    ~X(void) override {}

class TypeDeclaration: public Toplevel {
public:
    TypeDeclaration(const std::string &name, SourcePos pos)
        : Toplevel(ToplevelKind::TypeDeclaration, pos), _name(name) {}

    const std::string &name(void) const { return _name; }

    TOPLEVEL_CLASS(TypeDeclaration);
private:
    std::string _name;
};

class StructDeclaration: public Toplevel {
public:
    StructDeclaration(const std::string &name,
                      std::vector<std::unique_ptr<Declaration>> members,
                      SourcePos pos)
        : Toplevel(ToplevelKind::StructDeclaration, pos),
          _name(name),
          _members(std::move(members)) {}

    const std::string &name(void) const { return _name; }
    const std::vector<std::unique_ptr<Declaration>> &members(void) const {
        return _members;
    }

    TOPLEVEL_CLASS(StructDeclaration);
private:
    std::string _name;
    std::vector<std::unique_ptr<Declaration>> _members;
};

class TemplateStructDeclaration: public Toplevel {
public:
    TemplateStructDeclaration(
            const std::string &name,
            const std::vector<std::string> &argnames,
            std::vector<std::unique_ptr<Declaration>> members,
            SourcePos pos)
        : Toplevel(ToplevelKind::TemplateStructDeclaration, pos),
          _argnames(argnames),
          _decl(name, std::move(members), pos) {}

    const class StructDeclaration &decl(void) const { return _decl; }
    const std::vector<std::string> &argnames(void) const { return _argnames; }

    TOPLEVEL_CLASS(TemplateStructDeclaration);
private:
    std::vector<std::string> _argnames;
    class StructDeclaration _decl;
};

class FunctionDeclaration: public Toplevel {
public:
    FunctionDeclaration(const std::string &name,
                        std::vector<std::unique_ptr<Declaration>> args,
                        std::unique_ptr<Type> ret_type,
                        SourcePos pos)
        : Toplevel(ToplevelKind::FunctionDeclaration, pos),
          _name(name),
          _args(std::move(args)),
          _ret_type(std::move(ret_type)) {}

    const std::string &name(void) const { return _name; }
    const std::vector<std::unique_ptr<Declaration>> &args(void) const {
        return _args;
    }
    const Type &ret_type(void) const { return *_ret_type; }

    TOPLEVEL_CLASS(FunctionDeclaration);
private:
    std::string _name;
    std::vector<std::unique_ptr<Declaration>> _args;
    std::unique_ptr<Type> _ret_type;
};

class FunctionDefinition: public Toplevel {
public:
    FunctionDefinition(std::unique_ptr<class FunctionDeclaration> signature,
                       std::vector<std::unique_ptr<Statement>> &&block,
                       SourcePos pos)
        : Toplevel(ToplevelKind::FunctionDefinition, pos),
          _signature(std::move(signature)),
          _block(std::move(block)) {}

    const class FunctionDeclaration &signature(void) const {
        return *_signature;
    }
    const std::vector<std::unique_ptr<Statement>> &block(void) const {
        return _block;
    }

    TOPLEVEL_CLASS(FunctionDefinition);
private:
    std::unique_ptr<class FunctionDeclaration> _signature;
    std::vector<std::unique_ptr<Statement>> _block;
};

class TemplateFunctionDefinition: public Toplevel {
public:
    TemplateFunctionDefinition(
            std::unique_ptr<class FunctionDeclaration> signature,
            const std::vector<std::string> &argnames,
            std::vector<std::unique_ptr<Statement>> block,
            SourcePos pos)
        : Toplevel(ToplevelKind::TemplateFunctionDefinition, pos),
          _def(new class FunctionDefinition(std::move(signature),
                                            std::move(block), pos)),
          _argnames(argnames) {}

    std::shared_ptr<class FunctionDefinition> def(void) const { return _def; }
    const std::vector<std::string> &argnames(void) const { return _argnames; }

    TOPLEVEL_CLASS(TemplateFunctionDefinition);
private:
    std::shared_ptr<class FunctionDefinition> _def;
    std::vector<std::string> _argnames;
};

#undef TOPLEVEL_CLASS

template<typename Result>
class ToplevelVisitor {
public:
    virtual ~ToplevelVisitor(void) {}

    Result visit(const Toplevel &t) {
        switch (t.kind()) {
#define HANDLE(X) case Toplevel::ToplevelKind::X:\
                      return operator()(llvm::cast<X>(t));
            HANDLE(TypeDeclaration);
            HANDLE(StructDeclaration);
            HANDLE(TemplateStructDeclaration);
            HANDLE(FunctionDeclaration);
            HANDLE(FunctionDefinition);
            HANDLE(TemplateFunctionDefinition);
        }
#undef HANDLE
    }

private:
    virtual Result operator()(const TypeDeclaration &) = 0;
    virtual Result operator()(const StructDeclaration &) = 0;
    virtual Result operator()(const TemplateStructDeclaration &) = 0;
    virtual Result operator()(const FunctionDeclaration &) = 0;
    virtual Result operator()(const FunctionDefinition &) = 0;
    virtual Result operator()(const TemplateFunctionDefinition &) = 0;
};

void print_toplevel(const Toplevel &top, std::ostream &out);

}

}

