/**
 * @file Types.hh
 *
 * @brief The AST nodes that represent types.
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

namespace Craeft {

namespace AST {

class Type {
public:
    enum TypeKind {
        NamedType,
        Void,
        TemplatedType,
        Pointer
    };

    TypeKind kind(void) const {
        return _kind;
    }

    virtual ~Type() {}

    Type(TypeKind kind): _kind(kind) {}

private:
    TypeKind _kind;
};

#define TYPE_CLASS(X)\
    static bool classof(const Type *t) {\
        return t->kind() == TypeKind::X;\
    }\
    ~X(void) override {}

class NamedType: public Type {
public:
    const std::string& name(void) const { return _name; }
    NamedType(const std::string &name)
        : Type(TypeKind::NamedType), _name(name) {}

    TYPE_CLASS(NamedType);

private:
    std::string _name;
};

class Void: public Type {
public:
    Void(void): Type(TypeKind::Void) {}
    TYPE_CLASS(Void);
};

class TemplatedType: public Type {
public:
    const std::string &name(void) const { return _name; }
    const std::vector<std::unique_ptr<Type>> &args(void) const {
        return _args;
    }
    TemplatedType(const std::string &name,
                  std::vector<std::unique_ptr<Type>> &&args)
        : Type(TypeKind::TemplatedType),
          _name(name),
          _args(std::move(args)) {}
    TYPE_CLASS(TemplatedType);
private:
    std::string _name;
    std::vector<std::unique_ptr<Type>> _args;
};

class Pointer: public Type {
public:
    const Type &pointed(void) const { return *_pointed; }

    Pointer(std::unique_ptr<Type> pointed)
        : Type(TypeKind::Pointer), _pointed(std::move(pointed)) {}

    TYPE_CLASS(Pointer);
private:
    std::unique_ptr<Type> _pointed;
};

#undef TYPE_CLASS

template<typename Result>
class TypeVisitor {
public:
    virtual ~TypeVisitor() {};

    Result visit(const Type &type) {
        switch (type.kind()) {
#define HANDLE(X) case Type::TypeKind::X:\
                      return operator()(llvm::cast<X>(type));
            HANDLE(NamedType);
            HANDLE(Void);
            HANDLE(TemplatedType);
            HANDLE(Pointer);
#undef HANDLE
        }
    }

private:
    virtual Result operator()(const NamedType &) = 0;
    virtual Result operator()(const Void &) = 0;
    virtual Result operator()(const TemplatedType &) = 0;
    virtual Result operator()(const Pointer &) = 0;
};

void print_type(const Type &type, std::ostream &out);

}

}
