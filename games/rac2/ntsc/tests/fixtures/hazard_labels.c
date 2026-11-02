/* Authored cases for tests/test_hazard_labels.py (no game code). */

/* The fixed case: the reload of 1.0f (lui + mtc1) is followed by the join label that the
   first test's bc1tl reaches forward, and sub.s reads the reloaded register. */
void clamp(float *s, float *out)
{
    float x = s[15];
    if (x > 1.0f)
        x = 1.0f;
    else if (x < 0.0f)
        x = 0.0f;
    *out = 1.0f - x;
}

/* Control: an aligned loop head after the constant's mtc1, whose first instruction does
   not read it. */
void scale(float *v, int n)
{
    int i;
    for (i = 0; i < n; i++)
        v[i] = v[i] * 0.75f;
}

/* Control: a join label after a transfer in a branch-likely delay slot. */
float pick(float a, float b, int *p)
{
    float r = 0.0f;
    if (*p)
        r = a;
    return r + b;
}

/* Control: no coprocessor instruction at all. */
int count(int *p, int n)
{
    int c = 0;
    while (n-- > 0)
        if (*p++ > 3)
            c++;
    return c;
}
