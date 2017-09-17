fn unsigned_sub(U64 a, U64 b) -> U64 {
    return a - b;
}

fn signed_sub(I64 a, I64 b) -> I64 {
    return a - b;
}

fn float_sub(Float a, Float b) -> Float {
    return a - b;
}

fn double_sub(Double a, Double b) -> Double {
    return a - b;
}

fn ptr_int_sub(U8 *a, I64 b) -> U8 * {
    return a - b;
}

fn ptr_ptr_sub(U8 *a, U8 *b) -> I64 {
    return a - b;
}
