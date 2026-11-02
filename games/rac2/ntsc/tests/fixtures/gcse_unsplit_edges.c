/* Authored controls for the optional unsplit gcse; no game code. */
extern unsigned char table[256];
extern int pick(int);
extern void first(int);
extern void second(int);

/* The else arm is reached only by jumps (from both tests), so the edge from the first test
   into it is critical and needs no new jump: the Cygnus split gives the hoisted high half of
   `table` a block of its own on that edge; GCC 2.95 computes it in the entry block. */
void hoist_split(int a, int b)
{
    if (a == 0 && table[pick(0)] == 0)
        first(b);
    else
        second(b);
    table[pick(a)] += 1;
}

/* Control: a diamond, no critical edge. */
void diamond(int a, int b)
{
    if (a != 0)
        first(table[b]);
    else
        second(table[b + 1]);
    table[pick(a)] += 1;
}

/* Control: a loop invariant hoisted into the entry block. */
void invariant(int n)
{
    int i;

    for (i = 0; i < n; i++)
        table[pick(i)] += 1;
}
