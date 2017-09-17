fn fact(U64 n) -> U64 {
    if n == 0 {
        return 1;
    }
    return n * fact(n - 1);
}
