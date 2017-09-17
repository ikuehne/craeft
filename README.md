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
EBNF:

```
identifier: [a-z|non-ascii][a-zA-Z0-9_|non-ascii]*

TypeName: [A-Z][a-zA-Z0-9_|non-ascii]*

Type: TypeName
    | TypeName<:[Type,]* Type:>
    | Type *

op: [!*+-><&%^@~/=]+

expr: identifier
    | literal
    | expr op expr
    | identifier ( [expr,]* expr)
    | identifier ( )
    | Type ( expr )

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
supported, except for hexadecimal literals.  Double-quoted strings are also
supported.

An Example
----------

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
function on x86 is (after cleaning up a couple of labels and removing some
assembler directives):

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

Generics
--------

The main feature of Cr&#230;ft not shared with C is its support for structural
and functional generics.  A structural template allows filling struct fields
with type parameters.  So, for example, a generic linked list would look like:

```
struct<:T:> ListNode {
    T contents;
    U8 *next;
}
```

(there is a bug in the compiler implementation which prevents recursive struct
definitions, even where the size can be computed--this is an early alpha
compiler).  This can then be used like a normal struct by providing the type
parameter in a declaration:

```
ListNode<:U64:> int_list;
int_list.contents = 10;
...
```

Functional generics similarly allow for a function with a type parameter.  So,
if we wanted a function which created a new singleton list of a generic type, we
could write:

```
fn<:T:> list_new(T contents) -> ListNode<:T:> {
    ListNode<:T:> result;
    result.contents = contents;
    result.next = (U8 *)0;
    return result;
}
```

The compiler currently expands all templates at compile time, so this is
nominally a "zero-cost" abstraction.  Of course, that could easily lead to an
explosion in compile times and code size, so a short term goal is to add support
for runtime polymorphism, which has no such drawbacks.

Unicode
-------

Cr&#230;ft supports UTF-8 identifiers and strings.  The following code:

```
fn puts(U8 *str) -> I32;

fn main() -> I32 {
    U8 *cr&#230; = "&#128525;";
    return puts(cræft);
}
```

Prints

&#128525;

Linking with C Code
-------------------

Linking with C code is trivial.  The compiler uses the system C calling
convention and alignment rules, so compiled code need simply declare the C
function, and then it may be called freely.

Compiling
=========

To compile the compiler:

```
mkdir build
cd build
cmake ..
make
```

LLVM, a compiler supporting C++14, Boost, and cmake are required.

Usage and Example Code
======================

The compiler can produce LLVM IR, assembly, or static object code files, with
the `--ll`, `--asm`, and `-c` flags respectively.  So to compile the
`factorial.cr` example:

```
./craeftc ../examples/factorial.cr -c factorial.o
```

Most of the examples have a corresponding C "harness" which calls the Cr&#230;ft
functions.  So, to compile and run `factorial.cr` with the harness:

```
cc ../examples/factorial_harness.c -c
cc factorial_harness.o factorial.o -o factorial
./factorial
```

Testing
=======

Cr&#230;ft currently comes with a tiny suite of integration tests, in the
process of being expanded.  They may be run with the `test/integration/run.py`
script:

```
python3 test/integration/run.py
```

License
=======

Cr&#230;ft is distributed under the GPL3.  See [LICENSE](LICENSE).

[Boost](boost.org) and [LLVM](http://llvm.org) are distributed under their
respective licenses.
