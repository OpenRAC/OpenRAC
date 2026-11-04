/* Authored controls for the optional clear scheduling model; no game code. */
typedef int Quad __attribute__((mode(TI)));
extern void consume(void *, void *, int, float);
extern void touch(void *);

void isolated(Quad *p, float *value, void *context) {
    *p = 0;
    consume(p, context, 7, *value);
}

void pair(Quad *p, float *value, void *context) {
    p[0] = 0;
    p[1] = 0;
    consume(p, context, 7, *value);
}

void offset(Quad *p, float *value, void *context) {
    p[1] = 0;
    consume(p, context, 7, *value);
}

void negative_offset(Quad *p, float *value, void *context) {
    p[-1] = 0;
    consume(p, context, 7, *value);
}

void calls(Quad *p, float *value, void *context) {
    *p = 0;
    touch(p);
    *p = 0;
    consume(p, context, 7, *value);
}

void branch(Quad *p, float *value, void *context, int which) {
    if (which) p[0] = 0;
    else p[1] = 0;
    consume(p, context, 7, *value);
}
