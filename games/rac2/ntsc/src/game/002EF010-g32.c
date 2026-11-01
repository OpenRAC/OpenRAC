extern unsigned char D_001AD030[27] __attribute__((sda));
extern unsigned char D_001AD04B[] __attribute__((sda));

unsigned long func_002EF010(int a0)
{
    int i;
    int limit;
    unsigned char v;
    int out = (unsigned int)(a0 - 0x20) >= 0x91;

    if (!out) {
        limit = D_001AD04B[0];
        if (a0 >= limit) {
            a0 -= limit;
        } else {
            i = 0x1B;
        next:
            i--;
            if (i >= 0) {
                v = D_001AD030[i];
                if (a0 < v)
                    goto next;
                a0 -= v;
            }
        }
    } else if (a0 >= 0xB1) {
        a0 -= 0x91;
    }
    return a0;
}
