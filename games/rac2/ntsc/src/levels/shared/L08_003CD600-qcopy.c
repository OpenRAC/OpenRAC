typedef int Quadword __attribute__((mode(TI)));
extern char D_00189EA0[];
extern char *D_0018C0B0;
extern void func_002A0F48(char *);

/* The second argument is supplied by the caller but never read: the body reloads the source. */
void func_L08_003CD600(char *a0, Quadword *unused) {
    char *a1 = D_0018C0B0;

    *(Quadword *)(a0 + 0x10) = *(Quadword *)(a1 + 0x10);
    *(Quadword *)(a0 + 0xF0) = *(Quadword *)(a1 + 0xF0);
    func_002A0F48(a0);
}

void func_L08_003CD6C8(char *a0, Quadword *a1) {
    Quadword *transform = (Quadword *)D_00189EA0;

    Quadword *src;
    Quadword *dst;

    a1[0] = transform[0];
    src = transform + 1;
    dst = a1 + 1;
    *dst = *src;
    func_L08_003CD600(a0, a1);
}
