struct Point {
    Float x;
    Float y;
}

fn sqrt(Float x) -> Float;

fn init(Point *p, Float x, Float y) {
    p->x = x;
    p->y = y;
}

fn distance(Point *p) -> Float {
    return sqrt(p->x * p->x + p->y * p->y);
}
