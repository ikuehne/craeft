#include <stdio.h>

struct Point {
    float x;
    float y;
};

void init(struct Point *p, float x, float y);

float distance(struct Point *p);

int main(void) {
    struct Point p;

    init(&p, 4, 3);

    printf("Distance: %g.\n", distance(&p));
}
