/* Authored controls for the optional grouped-copy scheduling barrier; no game code.
   COPY is the plain C assignment the game units use. Built with -DASM_COPY (in this test
   only, never in src/), it is instead the volatile inline-asm copy that the barrier models. */
typedef int Quad __attribute__((mode(TI)));
#ifdef ASM_COPY
#define COPY(d, s) __asm__ __volatile__ ("lq $2,0(%1)\n\tsq $2,0(%0)" : : "r" (d), "r" (s) : "$2")
#else
#define COPY(d, s) (*(Quad *) (d) = *(const Quad *) (s))
#endif
extern void take_float(char *, float);
extern void take(char *);
extern void consume(char *, char *, char *, int);

/* A float constant, ready from the start of the block, against the copy's address stall. */
void constant_after(char *d, char *s) {
    COPY(d + 0x10, s + 0x20);
    take_float(d, 0.5f);
}

/* Independent loads before the copy, independent stores after it. */
void neighbours(char *d, char *s, int *p, int *q) {
    int a = p[1];
    int b = q[2];
    COPY(d, s);
    p[3] = a + 1;
    q[4] = b + 2;
}

/* The epilogue's restores against a copy whose address needs a chain of instructions. */
void restores_after(char *d, char *s, int *index) {
    take(d);
    COPY(d + *index * 16 + 0x80, s + 0x10);
}

/* A call's argument moves after the copy. */
void arguments_after(char *d, char *s, char *t, int n) {
    COPY(d, s + 0x10);
    consume(t, d, s, n);
}

/* A copy with no neighbours. */
void alone(char *d, char *s) {
    COPY(d, s);
}

/* Copies on both sides of a call. */
void around_call(char *d, char *s) {
    COPY(d, s);
    take(d);
    COPY(s, d);
}

/* A copy in each arm of a branch. */
void around_branch(char *d, char *s, int which, int *flag) {
    if (which)
        COPY(d, s);
    else
        COPY(d + 0x10, s);
    *flag = which + 1;
}

/* Volatile scalar accesses on both sides of a copy. */
void volatile_neighbours(volatile int *v, char *d, char *s) {
    int a = v[0];
    COPY(d, s);
    v[1] = a;
}

/* Two copies of a volatile quadword: both accesses of each are kept. */
void volatile_copy(volatile Quad *d, const volatile Quad *s) {
    *d = *s;
    *d = *s;
}
