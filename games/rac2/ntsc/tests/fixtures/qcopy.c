/* Authored aligned-copy controls; no game source or bytes. */
typedef int Quad __attribute__((mode(TI)));
extern Quad *resolve(void);
extern void observe(void *);

void pointers(Quad *dst, const Quad *src) { *dst = *src; }
void offsets(char *dst, const char *src) {
    *(Quad *)(dst + 16) = *(const Quad *)(src + 32);
}
void negative(char *dst, const char *src) {
    *(Quad *)(dst - 16) = *(const Quad *)(src + 32);
}
void scalar(int *dst, const int *src) { dst[4] = src[8]; }
void wide(long long *dst, const long long *src) { dst[2] = src[4]; }
void zero(Quad *dst) { dst[1] = 0; }
Quad *returned_address(const Quad *src) {
    Quad *dst = resolve();
    *dst = *src;
    return dst;
}
void volatile_access(volatile Quad *dst, const volatile Quad *src) {
    *dst = *src;
    *dst = *src;
}
void conditional(Quad *dst, const Quad *src, int copy) {
    if (copy) *dst = *src;
}
void local(Quad *dst, const Quad *src) {
    Quad pair[2];
    pair[1] = *src;
    observe(pair);
    *dst = pair[1];
}

void fp_conditional(Quad *dst, const Quad *src, float amount) {
    if (amount > 0.0f) *dst = *src;
}
void fp_choice(Quad *dst, const Quad *first, const Quad *second, float amount) {
    if (amount < 0.0f) *dst = *first;
    else *dst = *second;
}
