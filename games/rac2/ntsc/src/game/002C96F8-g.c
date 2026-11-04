/* Samples a keyframe track at time t with keys every step: the key index (clamped to
   the track, with the fraction zeroed at either end) goes to *idx and the fraction past
   it to *frac, and *out gets the key or the blend toward the next one. Retail compiles
   this with -g (the nop pair before div.s). */
typedef int TI128_2C96F8 __attribute__((mode(TI)));
typedef struct {
    int count;
    int pad4[3];
    float keys[1][4];
} KeyTrack2C96F8;
extern int func_00283CF0(float);
extern float func_00283CE0(int);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);

void func_002C96F8(float t, float step, KeyTrack2C96F8 *track, int *idx, float *frac, float *out)
{
    float delta[4];
    int i;

    i = func_00283CF0(t / step);
    *idx = i;
    if (i >= track->count - 1) {
        *idx = track->count - 1;
        *frac = 0.0f;
        *(TI128_2C96F8 *)out = *(TI128_2C96F8 *)track->keys[*idx];
    } else if (i < 0) {
        *idx = 0;
        *frac = 0.0f;
        *(TI128_2C96F8 *)out = *(TI128_2C96F8 *)track->keys[*idx];
    } else {
        *frac = t - func_00283CE0(i) * step;
        func_00282CF0(delta, track->keys[*idx + 1], track->keys[*idx]);
        *(TI128_2C96F8 *)out = *(TI128_2C96F8 *)delta;
        func_00282F20(out, out, *frac);
        func_00282CC0(delta, out, track->keys[*idx]);
        *(TI128_2C96F8 *)out = *(TI128_2C96F8 *)delta;
    }
}
