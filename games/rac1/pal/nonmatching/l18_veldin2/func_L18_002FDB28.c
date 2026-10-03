/* NON_MATCHING func_L18_002FDB28 -- src/overlays/l18_veldin2/vendor_002F9D48.c
 * Best so far: BYTES 31/372 (91.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Best candidate q6.c (22 runs); q5.c is the same but do/while, BYTES 31/372.
 *   ## Round 3 (1 run)
 *   `int ix = v; ix &= 0x7FFF;` (a separate pseudo for the masked value, hoping gcc
 *   would copy v out and mask in place as retail does) is coalesced away: identical
 *   bytes to q6. The missing `daddu $a3,$v0,$zero` is retail keeping the raw entry in
 *   a second register while masking the original in place; nothing source-level has
 *   reproduced it yet. Tried: two locals, reading `*p` twice (CSE, +8), `short v`
 *   (-8), `int ix` (no change).
 */
extern unsigned short *D_L18_001AC540[];
extern unsigned char *D_L18_00160058_m __asm__("D_L18_00160058") MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_L00_0025BC48(float *a, float *b, float *out, float speed, float g);
extern float func_L00_00258C80(float lo, float hi);

typedef struct {
    char pad0[0x1D0];
    char v[8];
    float f1D8;
    char pad1DC[0x10];
    float f1EC;
    char pad1F0[4];
    float f1F4;
} L18Data;
typedef struct {
    char pad00[0x10]; float pos[4]; unsigned char st; char pad21[3]; int *cls; char pad28[0xC]; unsigned short fl; char pad36[0x42]; L18Data *data; char pad7C[0x18]; int a94; char pad98[0xE]; short id;
} L18Moby;

int func_L18_002FDB28(int idx, float *pos, float *dir, float speed) {
    unsigned short *p = D_L18_001AC540[idx];
    if (p != 0) {
        unsigned char *base = D_L18_00160058_m;
        do {
            unsigned short v = *p;
            L18Moby *moby = (L18Moby *)(base + ((v & 0x7FFF) << 8));
            if (moby->id == 0x772 && moby->st == 8) {
                unsigned short fl = moby->fl;
                int *cls = moby->cls;
                L18Data *data = moby->data;
                moby->fl = fl & 0xFFBE;
                moby->a94 = cls[4];
                moby->fl = (fl & 0xFFBE) | 0x1000;
                qcopy(moby->pos, pos);
                moby->st = 1;
                data->f1F4 = 1.0f;
                func_001F9BF0(data->v, dir, pos);
                data->f1D8 = 0;
                func_L00_001FF4B0(data->v, data->v, speed);
                data->f1D8 = func_L00_0025BC48(pos, dir, 0, speed, -(D_0015EE70 * 10.0f));
                data->f1EC = func_L00_00258C80(15.0f, 45.0f) * 0.017453292f;
                return 1;
            }
            p++;
            if ((short)v < 0) break;
        } while (1);
    }
    return 0;
}
