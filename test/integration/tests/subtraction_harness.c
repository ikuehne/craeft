#include <stdio.h>
#include <stdint.h>

uint64_t unsigned_sub(uint64_t a, uint64_t b);
int64_t signed_sub(int64_t a, int64_t b);
float float_sub(float a, float b);
double double_sub(double a, double b);
char *ptr_int_sub(char *a, int64_t b);
int64_t ptr_ptr_sub(char *a, char *b);

int main(void) {
    printf("%llu\n", unsigned_sub(30, 10));
    printf("%lld\n", signed_sub(30, 10));
    printf("%g\n", float_sub(30, 10));
    printf("%g\n", double_sub(30, 10));
    char test_str[100] = "20";
    printf("%s\n", ptr_int_sub(test_str + 1, 1));
    char long_test_str[100] = "abcdefghijklmnopqrstuvwxyz";
    printf("%lld\n", ptr_ptr_sub(long_test_str + 20, long_test_str));
}
