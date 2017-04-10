//////////////////////////////////////////////////
// Comments.
//

// Everything between `//` and `\n` is treated as whitespace.

//////////////////////////////////////////////////
// Variable and type names.
//

// Variable names match [a-z_][a-zA-Z0-9_]*.
// Type names match [A-Z][a-zA-Z0-9_]*.
// Variable declarations are of the form `typename varname( = expression)?;`

//////////////////////////////////////////////////
// Integer types.
//

// Unsigned integer types are denoted `Un` where `n` is the number of bits.
// Any "n" from 1 to 64 is acceptable.

U19 x;
U1 bool;

// Signed integers are `In`
I32 x;

// Floating-point numbers may be `Float` (32 bits), or `Double` (64).
Float z;

// Integer literals are written in standard decimal notation.
U64 thousand = 1000;
I64 minus_1000 = -1000;

// Floating-point literals are as well.
Double pi = 3.14;

// Although scientific notation may also be used.
Double c = 3e8;

// Overflow is never checked (see below).  The only exception is that
// numeric literals may not be over 64 bits.
I32 x = (I32)1 << (I32)32;    // Compiles.
I32 x = (I32)2147483648;      // Compiles.
U64 x = 18446744073709551616; // Does not compile.

// Arithmetic expressions work as in C, except that C's increment, decement,
// compound assignment, and ternary operators are not supported, and
// assignments are not allowed in expressions.  The `&&` and `||` operators
// short-circuit.  `&&`, `||`, `==`, `!=`, `<=`, `<`, `>=`, and `>` all always
// evaluate to type U1.  Unary & evaluates to a pointer to the type of its
// operand.  Other operators evaluate to the type of their operands, which
// must be the same for binary operators.
I64 x = 1 << 30;                // x == 1073741824
U1 x = 1 == 1;                  // x == 1
I32 y;
I32 x = (y = 5);                // Fails to compile.
U1 z = (1 == 1) || (U1)(1 / 0); // z == 1.

// Casts are permitted between all integer and floating-point numbers.  Casts
// from floating-point to integral types are equivalent to rounding down, with
// the exception that any cast from a value too large to be represented in the
// target type gives an undefined value.  Casts are not made implicitly.
Double w = 3.14;
I64 x = w;      // Fails to compile: all casts must be explicit.
I64 x = (I64)w; // x == 3.
U1  y = (U1)w;  // Compiles, but y's value is undefined.
U1  z = (U1)x;  // Compiles, but z's value is undefined.

// The type of floating-point literals is assumed to be Double.  The type of
// positive integer literals is assumed to be U64.  The type of negative
// integer literals is assumed to be I64.  

// The address operator may only be applied to lvalues (that is, variables,
// dereferences of variables, and indices into lvalues).

// The same overflow behavior used in casts is used in all other arithmetic
// operations: if the result value is too large to hold in the result type, the
// result value is undefined.
I32 x = (I32)1 << (I32)32; // Compiles, but x's value is undefined.

///////////////////////////////////////////////
// Statements.
//

// A statement is:
//  - `typename varname;` (discussed above)
//  - `expression;` (which evaluates the expression)
//  - `varname = expression;` (which sets the variable `varname` to the value
//    of the expression).
//  - `typename varname = expression;` (which declares variable `varname`, and
//    then does the same as the above).
//  - `return expression` (see below).

///////////////////////////////////////////////
// Functions.
//

// A function consists of a declaration and a body.  The declaration is of the
// form `fn varname(typename varname, ...) [-> typename]`. The body is a brace-
// enclosed sequence of statements.  Functions return when a `return` statement
// is evaluated.  No statements may follow the return statement in a brace-
// enclosed block.  The expression returned must be of the type specified after
// the arrow in the declaration.  A function may have no return type, in which
// case no `-> type` shall be given, and the `return` statement takes no
// argument.

fn double(Double d) -> Double {
    return d * 2;
}

Double d = 1.5;
d = double(d);  // d == 3

fn double(Double d) -> Double {
    return d * 2;
    return d * 3; // Fails to compile.
}

fn noop(Double x) {
    x = x + 1;
    return;
}

fn noop2(Double x) {
    x = x + 1;
}

// Functions may be declared without being used.
fn malloc(U64 size) -> U8 *;

///////////////////////////////////////////////
// Function pointers.
//

// Function types are denoted (for example, in a struct):
struct Hashable<T> {
    (T) -> I64 hash;
}

struct Addable<T> {
    (T, T) -> T add;
}

// Similarly, as an argument to a function (functions cannot be return types):
fn qsort(I64 *arr, U64 size, (I64, I64) -> U1 comp);

///////////////////////////////////////////////
// Conditionals.
//

// Conditionals take the form `if (expression) { statement ... }`.
fn double_if_odd(I64 x) -> I64 {
    if (x % 2 == 1) {
        return x * 2;
    }
    return x;
}

// The condition must be a U1.
fn double_if_odd(I64 x) -> I64 {
    if (x % 2) {      // Does not compile.
        return x * 2;
    }
    return x;
}

// An optional `else` block may be present, which executes only if the
// condition is not true.

fn double_if_odd(I64 x) -> I64 {
    if (x % 2 == 1) {
        return x * 2;
    } else {          // Equivalent to the first double_if_odd.
        return x;
    }
}

//////////////////////////////////////////////////
// Loops.
//

// "while" loops take the form `while (expression) { statement ... }`.

fn fibonacci(U64 n) -> U64 {
    U64 f0 = 0;
    U64 f1 = 1;
    U64 tmp;
    U64 i = 1;
    while (i < n) {
        tmp = f0;
        f0 = f1;
        f1 = tmp + f1;
        i = i + 1;
    }
    return f0;
}

fibonacci(4) // Evaluates to 2.

//////////////////////////////////////////////////
// Pointers.
//

// Pointer types are denoted as in C.
U64 *p;

// A pointer is an address in memory.  Casting it to an integer with the same
// size as the processor's address width should give a valid address, which can
// be casted back to the same pointer.  All other casts to and from pointers
// have undefined values.  NULL is defined as in C, as is the `sizeof` operator,
// which returns an unsigned int of the same size as the target's address width.

// On a 64-bit machine:
I64 *x = [some address];
I64 y = (I64)x;
I64 *z = (I64 *)y;
*z == *x; // --> 1.
z == x;   // --> 1.
*z = 5;
z == x;   // --> 1.
*z == *x; // --> 1.

// Any expression can have its address taken with the `&` operator.
fn swap(I64 *a, I64 *b) {
    I64 tmp = *a;
    *a = *b;
    *b = tmp;
}

I64 x = 1;
I64 y = 2;
swap(&x, &y);
x; // --> 2
y; // --> 1

///////////////////////////////////////
// Structs.
//

struct Point {
    Double x;
    Double y;
// Note no semicolon.
}

// Note that struct types do not need to be prefaced with `struct` in
// declarations.
Point p {0, 1};
p.x; // --> 0
p.y; // --> 1

// There is a `->` operator as in C.
(&p)->x; // --> 0
(&p)->y; // --> 1

// Types may be declared without being defined using the "type" keyword.
type Point;
type IntList;

// Pointers to such types may then be used in type definitions.
struct IntList {
    I64 contents;
    IntList *next;
}

///////////////////////////////////////
// Generics.
//

// Structs and functions may be parameterized on other types.

// A linked-list implementation:
fn malloc(U64 size) -> U8 *;

struct<T> ListNode {
    T contents;
    ListNode<T> *next;
    ListNode<T> *prev;
}

struct<T> Hashable {
    T -> I64 hash;
}

fn hash_int(I64 x) -> I64 {
    return x;
}

Hashable<I64> hashable_int {hash_int};

typeclass<T> Hashable {
    T -> I64 hash;
}

struct<T> List {
    ListNode<T> *start;
    ListNode<T> *end;
}

fn<T> list_new() -> List<T> * {
    List<T> result = malloc(sizeof(List<T>));
    result->start = NULL;
    result->end = NULL;
    return result;
}

fn<T> list_append(List<T> *l, T contents) {
    ListNode<T> *new = malloc(sizeof(ListNode<T>));
    new->contents = contents;
    new->next = NULL;
    new->prev = l->end;
    l->end->next = new;
    l->end = new;
}

fn<T> list_prepend(List<T> *l, T contents) {
    ListNode<T> *new = malloc(sizeof(ListNode<T>));
    new->contents = contents;
    new->prev = NULL;
    new->next = l->start;
    l->start->prev = new;
    l->start = new;
}

fn<T> list_start(List<T> l) -> T {
    return l.start->contents;
}

fn<T> list_end(List<T> l) -> T {
    return l.end->contents;
}

fn main() {
    List<Int64> *l = list_new<Int64>;
    
    list_append<Int64>(l, 10);
    list_prepend<Int64>(l, 20);

    list_start(*l); // --> 20
    list_end(*l);   // --> 10
    // (leaks memory).
}

// A dynamic array implementation.
fn realloc(U8 *original, U64 size) -> U8 *;

struct<T> Vector {
    T *data;   
    U64 size;
    U64 capacity;
}

fn<T> new_vector(U64 reserved) -> Vector<T> * {
    Vector<T> *result = malloc(sizeof(Vector<T>));
    result.data = malloc(sizeof(T) * reserved);
    result.size = 0;
    result.capacity = reserved;
}

fn<T> vec_append(Vector<T> *vec, T new) {
    if (vec->size == vec->capacity) {
        vec->capacity = vec->capacity * 2;
        vec->data = realloc(vec->data, vec->capacity);
    }
    vec->data[vec->size] = new;
    vec->size = vec->size + 1;
}

fn<T> vec_pop(Vector<T> *vec) -> T {
    vec->size = vec->size - 1;
    if (vec->size  * 2 < vec->capacity) {
        vec->capacity = vec->capacity / 2;
        vec->data = realloc(vec->data, vec->capacity);
    }
    vec->size = vec->size - 1;
    return vec->data[vec->size];
}
