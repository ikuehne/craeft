#include <stdio.h>
#include <stdint.h>

uint64_t unsigned_add(uint64_t a, uint64_t b);
int64_t signed_add(int64_t a, int64_t b);
float float_add(float a, float b);
double double_add(double a, double b);
char *ptr_int_add(char *a, int64_t b);
char *int_ptr_add(int64_t a, char *b);

int main(void) {
    printf("%llu\n", unsigned_add(10, 10));
    printf("%lld\n", signed_add(10, 10));
    printf("%g\n", float_add(10, 10));
    printf("%g\n", double_add(10, 10));
    char test_str[100] = "     20";
    printf("%s\n", ptr_int_add(test_str, 5));
    printf("%s\n", int_ptr_add(5, test_str));
}
