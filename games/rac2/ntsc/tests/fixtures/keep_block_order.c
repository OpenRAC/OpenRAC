/* Authored controls for the optional block order; no game code. */
extern void call(int);
extern int value(int);
extern void after(void);

/* A switch whose default is its last case: the dispatch ends in `b default`, and the Cygnus
   block merge moves the default body up behind it; GCC 2.95 keeps it last. */
void default_last(int v)
{
    switch (v) {
    case 6:
        call(8);
        break;
    case 11:
        call(9);
        break;
    default:
        call(value(5) + 2);
        break;
    }
    after();
}

/* Control: the default first. */
void default_first(int v)
{
    switch (v) {
    default:
        call(value(5) + 2);
        break;
    case 6:
        call(8);
        break;
    case 11:
        call(9);
        break;
    }
    after();
}

/* Control: an if/else chain. */
void chain(int v)
{
    if (v == 6)
        call(8);
    else if (v == 11)
        call(9);
    else
        call(value(5) + 2);
    after();
}

/* Control: a loop. */
void loop(int n)
{
    int i;

    for (i = 0; i < n; i++)
        call(value(i));
    after();
}
