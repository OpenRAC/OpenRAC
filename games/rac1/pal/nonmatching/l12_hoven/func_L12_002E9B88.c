/* NON_MATCHING func_L12_002E9B88 -- src/overlays/l12_hoven/vendor_002C0310.c
 * Best so far: BYTES 39/1032 (96.2% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Hoven moby update: a timed aim check (data+0xF4 and data+0xD0 calls, a 0x26C table lookup, and a 0x243/0x242 c
 *   Also mv (moby+0x10) is computed into $s4 directly where retail copies it through $a0. Unblock: a source shape 
 */
extern float func_L00_001FF860(float, float);
extern float func_L00_0025CE58(void *, float, void *, float, float, float);
extern float func_001F9D48(void *, void *);
extern float func_001FA748(float, float);
extern float func_001FA850(float, float);
extern void func_00214D28(void *, float, float);
extern void func_L12_00272D90(void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_00262BC0(int, void *, void *, void *);
extern int func_L00_0025A778(void *, void *, int);
extern float func_001F9D10(void *, void *);
extern int func_001F9850(int);
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern char *D_L12_001B0C30[];

/* Moby update: aims at pos, runs the timed check on data[0x242]/[0x243], and writes the result back to moby+0x10. */
void func_L12_002E9B88(char *moby, float *pos, float a, float b) {
    unsigned char *data = *(unsigned char **)(moby + 0x78);
    float vec[4];
    float out[4];
    float V2[4];
    float W[4];
    float r, t, f22, q, g;
    char *mv;
    int v, c, ret;

    if (data[0x242] != 0) {
        char *m48 = moby + 0x48;
        r = func_L00_001FF860(pos[0] - *(float *)(moby + 0x10), pos[1] - *(float *)(moby + 0x14));
        func_L00_0025CE58(m48, r, data + 0x23C, D_0015EE70 * 15.707963f, D_0015EE70 * 12.566371f, D_0015EE6C * 25.1327419f);
        return;
    }

    mv = moby + 0x10;
    f22 = func_001F9D48(mv, pos);
    if (f22 > 6.5f && a != 0.0f) {
        r = func_L00_001FF860(pos[0] - *(float *)(moby + 0x10), pos[1] - *(float *)(moby + 0x14));
        t = func_001FA748(r, a);
        func_L00_0025CE58(moby + 0x48, t, data + 0x23C, D_0015EE70 * 15.707963f, D_0015EE70 * 12.566371f, D_0015EE6C * 25.1327419f);
    } else {
        char *m48 = moby + 0x48;
        r = func_L00_001FF860(pos[0] - *(float *)(moby + 0x10), pos[1] - *(float *)(moby + 0x14));
        func_L00_0025CE58(m48, r, data + 0x23C, D_0015EE70 * 15.707963f, D_0015EE70 * 12.566371f, D_0015EE6C * 25.1327419f);
    }

    if (b != 0.0f) {
        func_00214D28(data + 0xF4, b, D_0015EE70 * 7.0f);
    } else {
        q = func_L00_001FF860(pos[0] - *(float *)(moby + 0x10), pos[1] - *(float *)(moby + 0x14));
        g = func_001FA850(*(float *)(moby + 0x48), q);
        if (g < 1.57079637f && 6.0f < f22) {
            func_00214D28(data + 0xF4, D_0015EE6C * 7.0f, D_0015EE70 * 7.0f);
        } else {
            func_00214D28(data + 0xF4, D_0015EE6C, D_0015EE70 * 7.0f);
        }
    }

    *(u128 *)vec = *(u128 *)(moby + 0x10);
    func_L12_00272D90(moby, data + 0xD0);
    if (*(int *)(data + 0x26C) == -1) {
        return;
    }
    func_001F9BF0(W, vec, mv);
    func_L00_001FF4B0(W, W, 0.05f);
    func_001F9BD8(V2, vec, W);
    if (func_L00_00262BC0(*(int *)(data + 0x26C), mv, V2, out) == 0) {
        return;
    }
    if (func_L00_0025A778(mv, D_L12_001B0C30[*(int *)(data + 0x26C)] + 0x10,
                          *(int *)D_L12_001B0C30[*(int *)(data + 0x26C)]) == 0) {
        *(u128 *)mv = *(u128 *)out;
    }
    r = func_001F9D10(mv, vec);
    if (r < D_0015EE6C * 0.5f) {
        v = data[0x243] + 1;
        data[0x243] = v;
        c = v & 0xFF;
        ret = func_001F9850(15);
        if (ret < c) {
            data[0x242] = 1;
        }
    } else {
        data[0x243] = 0;
    }
}
