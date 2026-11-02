extern int D_001A748C __attribute__((sda));
extern int D_001A7490 __attribute__((sda));
extern int func_001245D0(void);
int func_001336C0(int a0) {
    int old;
    if (D_001A748C == 0)
        return func_001245D0();
    old = D_001A7490;
    D_001A7490 = a0;
    return old;
}
