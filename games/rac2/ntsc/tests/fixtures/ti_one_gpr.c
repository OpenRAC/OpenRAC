/* Authored controls for ti_one_gpr.py (one general register per TImode reload); no game code. */
typedef int Quad __attribute__((mode(TI)));
extern void touch(void *);
extern int value(int);

struct Frame { int a, b, c, d; long long e, f; };

/* Five displaced zero stores between calls, the first beyond a 16-bit offset: the address
   reload adds spill registers, and each zero's reload takes the next one round-robin. */
void rotate(Quad *p) {
    p[3000] = 0;
    touch(p);
    p[1] = 0;
    touch(p);
    p[2] = 0;
    touch(p);
    p[3] = 0;
    touch(p);
    p[4] = 0;
}

/* A shared zero for two stack quadwords while a call's value and two constants are live. */
void stack(int n) {
    Quad a[2];
    struct Frame w;
    long long v;

    touch(&w);
    v = value(n);
    w.e = v;
    w.b = 0x7f103080;
    a[0] = 0;
    a[1] = 0;
    w.f = v;
    w.c = 0x3f103080;
    touch(a);
    touch(&w);
}

/* One displaced zero store: the first spill register, with or without groups. */
void single(Quad *p) {
    p[1] = 0;
}
