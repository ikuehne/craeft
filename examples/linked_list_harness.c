#include <stdio.h>

void *double_stack_new(void);
void double_stack_push(void *stack, double);
double double_stack_pop(void *stack);

int main(void) {
    void *stack = double_stack_new();
    double_stack_push(stack, 5.0);
    double_stack_push(stack, 6.0);
    double_stack_push(stack, 7.0);

    printf("Should be 7, 6, 5: %g, %g, %g.\n", double_stack_pop(stack),
                                               double_stack_pop(stack),
                                               double_stack_pop(stack));
}
