/* Authored controls for the optional null-pointer-check pass; no game code. */
extern void call(int);

/* The pointer is dereferenced before the test: the pass deletes the test as always false. */
int checked_after_load(int *p)
{
    int x = *p;
    if (p == 0)
        call(1);
    return x;
}

/* Control: the test comes first, so the pass has nothing to delete. */
int checked_before_load(int *p)
{
    if (p == 0)
        call(1);
    return *p;
}
