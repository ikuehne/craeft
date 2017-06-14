Cr&#230;ft
==========

This is a compiler for a new systems programming language, written for Caltech
CS 81 with Donnie Pinkston as project mentor. The language, Cr&#230;ft, is a
strongly statically typed, imperative, low-level programming language with
support for generic programming inspired by C and Rust.

Internal documentation for the compiler can be found
[here](http://iankuehne.com/craeft).

Language
========

Cr&#230;ft is a systems programming language based on:
- Low-level access to the hardware.  A competent programmer should understand
  how their code is executed on the processor.  In practice, this means that
  memory is manually managed, and the programmer can make unlimited use of
  pointers.
- Low-cost abstractions.  The programmer should not have to balance performance
  with maintainable code.
- Strong static typing.  The type system should be expressive, and the compiler
  should only allow it to be circumvented with explicit annotations.

Syntax
------

The syntax is simple, and largely familiar to any C programmer.  Roughly, in
EBNR:

```
identifier: [a-z][a-zA-Z0-9_]*

Type: [A-Z][a-zA-Z0-9_]*

op: [!*+-><&%^@~/=]+

expr: identifier
    | literal
    | expr op expr

declaration: Type identifier;
           | Type identifier = expr;

statement: declaration
         | expr;
         | ifblock
         | return expr;

ifblock: if expr { statement* }
       | if expr { statement* } else { statement* }

arglist: ( )
       | ([Type identifier,]* Type identifier)

typelist: <: :>
        | <: [Type,]* Type :>

signature: fn identifier arglist -> Type
         | fn typelist identifier arglist -> Type

func: signature;
    | signature { statement* }

struct: struct Type { [Type identifier;]* }
      | struct typelist Type { [Type identifier;]* }

toplevel: func | struct

program: toplevel*
```

This omits a number of details important to the semantics, in particular which
operators are actually allowed (all C operators except the ternary operator and
the assignment operators) and the precedences and fixities (which follow C).
I've also omitted the definition of `literal`, simply because it is boring and
new literals are likely to be added soon.  C numeric literals are currently
supported, except for hexadecimal literals.

### An Example

As is traditional, we introduce the language with a definition of the factorial
function:

```
fn fact(U64 n) -> U64 {
    if n == 0 {
        return 1;
    }
    return n * fact(n - 1);
}
```

A couple of interesting features of the language are present here: integer types
are written with a `U` or an `I` followed by the number of bits, which can range
from 1 (for a boolean) to 64.

Also, the compiler optimizes simple tail calls.  The code it produces for this
function is (after cleaning up a couple of labels and removing some assembler
directives):

```assembly_x86
fact:
    movl    $1, %eax
    jmp     finish

loop:
    imulq   %rdi, %rax
    decq    %rdi

finish:
    testq   %rdi, %rdi
    jnz     loop
    retq
```

