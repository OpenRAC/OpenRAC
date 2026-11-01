/* Authored controls for tests/test_reload_reg_used_guard.py. Eleven pointers live across the
   calls take the callee-saved registers, so the count n lives in a stack slot. */
extern int g(int);
extern int *q[];
extern int *P[12];
extern struct { int pad[6]; int *hit; } C;
#define S (*a + *b + *c + *d + *e + *f + *h + *i + *j + *l + *m)

/* After n++ the count is in the spill register $v0, which local-alloc also gives the slt result:
   it is not a spill register of the slt, so reload loads n again instead of inheriting it. */
int refused(void)
{
    int *a = P[0], *b = P[1], *c = P[2], *d = P[3], *e = P[4], *f = P[5];
    int *h = P[6], *i = P[7], *j = P[8], *l = P[9], *m = P[10];
    int *seen[128];
    int n = 0, k = 0;
    while (q[k] != 0) {
        seen[k & 127] = q[k];
        g(S); g(S); g(S);
        {
            int *hit = C.hit;
            if (hit && *hit == 5) {
                n++;
                hit[38] = 0;
                if (n > 127)
                    break;
            }
        }
        g(S); g(S); g(S);
        k++;
    }
    for (k = 0; k < n; k++)
        *seen[k] = 1;
    return 0;
}

/* Here the increment's reload register is a spill register of the slt too: inherited. */
int inherited(void)
{
    int *a = P[0], *b = P[1], *c = P[2], *d = P[3], *e = P[4], *f = P[5];
    int *h = P[6], *i = P[7], *j = P[8], *l = P[9], *m = P[10];
    int *seen[128];
    int n = 0, k = 0;
    while (q[k] != 0) {
        g(S); g(S); g(S);
        {
            int *hit = C.hit;
            if (hit && *hit == 5) {
                seen[n++] = hit;
                hit[38] = 0;
                if (n > 127)
                    break;
            }
        }
        g(S); g(S); g(S);
        k++;
    }
    for (k = 0; k < n; k++)
        *seen[k] = 1;
    return 0;
}
