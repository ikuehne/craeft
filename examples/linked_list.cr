fn malloc(I64 x) -> U8 *;
fn free(U8 *x);

fn<:T:> sizeof() -> U64 {
    T *start = (T *)0;
    T *end = start + 1;
    return ((U8 *)end) - ((U8 *)start);
}

struct<:T:> ListNode {
    T contents;
    U8 *next;
}

struct<:T:> Stack {
    ListNode<:T:> *tos;
}

fn<:T:> stack_new() -> Stack<:T:> *{
    Stack<:T:> *result = (Stack<:T:> *)malloc((I64)8);
    result->tos = (ListNode<:T:> *)0;
    return result;
}

fn<:T:> stack_push(Stack<:T:> *stack, T x) {
    ListNode<:T:> *new = (ListNode<:T:> *)malloc((I64)sizeof<: ListNode<:T:> :>());
    new->next = (U8 *)stack->tos;
    stack->tos = new;
    new->contents = x;
}

fn<:T:> stack_pop(Stack<:T:> *stack) -> T {
    T result = stack->tos->contents;
    ListNode<:T:> *old = stack->tos;
    stack->tos = (ListNode<:T:> *)old->next;
    free((U8 *)(old));
    return result;
}

fn double_stack_new() -> Stack<:Double:> *{
    return stack_new<:Double:>();
}

fn double_stack_push(Stack<:Double:> *stack, Double x) {
    stack_push<:Double:>(stack, x);
}

fn double_stack_pop(Stack<:Double:> *stack) -> Double {
    return stack_pop<:Double:>(stack);
}
