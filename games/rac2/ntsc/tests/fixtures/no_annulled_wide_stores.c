/* Authored cases for tests/test_no_annulled_wide_stores.py (no game code). */
typedef int TI128 __attribute__((mode(TI)));

extern void use(void *);
extern void use2(void *, int);

/* the thread path, 128 bits: a loop that starts with a quadword clear */
void thread_sq(int n)
{
    float v[4];
    int i;

    for (i = 0; i < n; i++) {
        *(TI128 *)v = 0;
        v[2] = 1.0f;
        use(v);
    }
}

/* the thread path, 64 bits: a loop that starts with a doubleword clear */
void thread_sd(long long *p, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        *p = 0;
        use(p);
    }
}

/* the steal path (with -fno-thread-jumps): the loop branch takes the doubleword store from the
   delay list of the skip branch at its target */
void steal_sd(long long *p, long long v, int *q, int *r)
{
    int x = *q;

    while (x != 0) {
        if (x != 0)
            *p = v;
        r[2] = x;
        x = *++q;
    }
}

/* control: the same loop with a word clear keeps its annulled store */
void thread_sw(int *p, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        *p = 0;
        use(p);
    }
}

/* control: optimize_skip annuls the one doubleword store a branch skips */
void skip_sd(long long *p, long long v, int a)
{
    if (a == 0)
        *p = v;
}

/* control: a doubleword store fills a conditional branch's ordinary slot from before it */
void branch_slot_sd(long long *p, long long v, int a, int *r)
{
    *p = v;
    if (a)
        r[0] = 1;
}

/* control: an unconditional jump takes the join's quadword clear into its ordinary slot */
void jump_slot_sq(int a)
{
    float v[4];

    if (a)
        use2(v, 1);
    else
        use2(v, 2);
    *(TI128 *)v = 0;
    use(v);
}
