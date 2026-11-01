/* Authored controls for -frac2-zero-ti-por (plain C quadword zero stores); no game code. */
typedef int Quad __attribute__((mode(TI)));
extern void consume(void *, void *, int, float);
extern void lanes(float *, float *, float *);
extern void touch(void *);
extern int other(Quad *);
extern Quad table[4];

/* A local vector at 0($sp), cleared and then partly set: retail's plain C clear. */
void stack(float *value, void *context) {
    float v[4];

    *(Quad *)v = 0;
    v[2] = *value;
    consume(v, context, 7, *value);
}

/* The same clear in a loop. */
void stack_loop(float *value, void *context, int n) {
    while (n-- > 0) {
        float v[4];

        *(Quad *)v = 0;
        v[1] = *value;
        consume(v, context, n, *value);
    }
}

/* A zero stored through a pointer register at displacement 0. */
void pointer(Quad *p, float *value, void *context) {
    *p = 0;
    consume(p, context, 7, *value);
}

/* A zero stored to a global, whose address is formed in a register. */
void global(float *value, void *context) {
    table[0] = 0;
    consume(table, context, 7, *value);
}

/* A pointer clear and a stack clear in one function. */
void mixed(Quad *p, float *value, void *context) {
    float v[4];

    *p = 0;
    touch(p);
    *(Quad *)v = 0;
    v[2] = *value;
    consume(v, context, 7, *value);
}

/* The stack clear below other locals, at a nonzero displacement. */
void stack_offset(float *value, void *context) {
    float a[4];
    float v[4];

    lanes(a, a, a);
    *(Quad *)v = 0;
    v[2] = *value;
    consume(v, a, 7, *value);
}

/* A field at a nonzero displacement from a pointer register. */
void pointer_offset(Quad *p, float *value, void *context) {
    p[1] = 0;
    consume(p, context, 7, *value);
}

/* One materialised zero feeding several stores. */
void several(Quad *p, Quad *q, float *value, void *context) {
    *p = 0;
    *q = 0;
    p[2] = 0;
    consume(p, q, 7, *value);
}

/* A zero that is also the function value: the default folds the store and sets the value again. */
int value(Quad *p, int c) {
    if (c == 0)
        return other(p);
    *p = 0;
    return 0;
}

/* A zero that stays live after its store (the function value, past a join): never folded. */
int live(Quad *p, int c) {
    if (c) {
        *p = 0;
        return 0;
    }
    return other(p);
}

/* A pointer clear in a counted loop. */
void loop(Quad *p, int n) {
    while (n-- > 0) {
        *p = 0;
        p += 2;
    }
}
