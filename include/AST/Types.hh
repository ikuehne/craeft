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

#include "Error.hh"

namespace Craeft {

namespace AST {

/**
 * @brief Syntactic representation of types.
 *
 * Uses LLVM RTTI.
 */
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

    SourcePos pos(void) const { return _pos; }

    virtual ~Type() {}

    Type(TypeKind kind, SourcePos pos): _kind(kind), _pos(pos) {}

private:
    TypeKind _kind;
    SourcePos _pos;
};

#define TYPE_CLASS(X)\
    static bool classof(const Type *t) {\
        return t->kind() == TypeKind::X;\
    }\
    ~X(void) override {}

/**
 * @brief A concrete type referenced by a name.
 */
class NamedType: public Type {
public:
    const std::string& name(void) const { return _name; }
    NamedType(const std::string &name, SourcePos pos)
        : Type(TypeKind::NamedType, pos), _name(name) {}

    TYPE_CLASS(NamedType);

private:
    std::string _name;
};

/**
 * @brief Void type (often not named, as in void-valued functions).
 */
class Void: public Type {
public:
    Void(SourcePos pos): Type(TypeKind::Void, pos) {}
    TYPE_CLASS(Void);
};

/**
 * @brief A type template referenced by a name.
 */
class TemplatedType: public Type {
public:
    const std::string &name(void) const { return _name; }
    const std::vector<std::unique_ptr<Type>> &args(void) const {
        return _args;
    }
    TemplatedType(const std::string &name,
                  std::vector<std::unique_ptr<Type>> &&args,
                  SourcePos pos)
        : Type(TypeKind::TemplatedType, pos),
          _name(name),
          _args(std::move(args)) {}
    TYPE_CLASS(TemplatedType);
private:
    std::string _name;
    std::vector<std::unique_ptr<Type>> _args;
};

/**
 * @brief A pointer type.
 */
class Pointer: public Type {
public:
    const Type &pointed(void) const { return *_pointed; }

    Pointer(std::unique_ptr<Type> pointed, SourcePos pos)
        : Type(TypeKind::Pointer, pos), _pointed(std::move(pointed)) {}

    TYPE_CLASS(Pointer);
private:
    std::unique_ptr<Type> _pointed;
};

#undef TYPE_CLASS

/**
 * @brief Visitor for type ASTs, parameterized on the return type.
 *
 * Derived classes should override `operator()`.
 */
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

/**
 * @brief Pretty-print the given AST type to the given stream.
 */
void print_type(const Type &type, std::ostream &out);

}
}
