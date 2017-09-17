#include <stdio.h>
#include <stdint.h>

uint64_t fact(uint64_t n);

int main(void) {
    printf("%llu\n", fact(10));
    return 0;
}
