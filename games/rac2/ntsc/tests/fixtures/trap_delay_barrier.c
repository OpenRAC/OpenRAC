/* Fixture for tests/test_trap_delay_barrier.py: authored, not game code. */
extern void step(int kind, int frame, float rate);
extern unsigned char *table;

/* The call's first argument is set before the division guard. */
void guarded_call(unsigned char *o, int n)
{
    if (o[3] != 2)
        step(2, (o[1] * n / table[4] + 1) % n, 8.0f);
}

/* Control: no division, so no guard; the slot is filled as usual. */
void plain_call(unsigned char *o, int n)
{
    if (o[3] != 2)
        step(2, o[1] * n + 1, 8.0f);
}
