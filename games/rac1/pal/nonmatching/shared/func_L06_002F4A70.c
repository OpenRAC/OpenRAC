/* NON_MATCHING func_L06_002F4A70 -- src/overlays/shared/vendor_002D9548.c
 * Best so far: SIZE ours 400 / retail 396, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Starts a charge-up (state 9) on a moby when a target (func_L00_0025B478 class 0x330001) differs from the playe
 *   p1.c: only the first target compare differs: ours `beql` with swc1 (f store) hoisted above, retail plain `beq`
 *   Unblock: the shape of the shared 0xFF tail (retail has two entry labels, one with the addiu in the delay slot)
 */
typedef int u128 __attribute__((mode(TI)));
extern char *a_find(void *, int, int) __asm__("func_L00_0025B478");
extern float a_ang(float, float) __asm__("func_L00_001FF860");
extern int a_rnd(int) __asm__("func_001F9850");
extern void a_set(float *, float, float) __asm__("func_L03_00251A58");
extern void a_pos(void *, float *, void *, void *) __asm__("func_L00_0025BBA0");
extern void a_turn(float, void *, void *, int, int, int) __asm__("func_L00_0025D5B0");
extern void a_snd(int, int, int) __asm__("func_0022ED80");
extern float D_0015EE6C MACRO_ADDR;
extern char D_0013E633[];

/* Starts a charge-up on a moby that has a valid target. */
void func_L06_002F4A70(char *self) {
    float v[4];
    float f;
    char *data = *(char **)(self + 0x78);
    if (((unsigned char *)self)[0x20] != 9 && ((unsigned char *)self)[0x52] != 2 && ((unsigned char *)self)[0x53] != 2) {
        char *o = a_find(self, 0x330001, 0);
        if (o != 0) {
            int p, q;
            char *t;
            f = a_ang(*(float *)(o + 0x10), *(float *)(o + 0x14));
            t = *(char **)(o + 0x20);
            if (t == *(char **)(D_0013E633 + 0x2E9D)) {
            } else if (*(short *)(t + 0xA6) == *(short *)(self + 0xA6)) {
            } else {
                char *s = data + 0x80;
                p = a_rnd(0x50);
                q = a_rnd(0x50);
                p = p * q;
                data[0xBD] = 0;
                *(int *)(data + 0xA4) = 9;
                *(float *)(data + 0x90) = 4.0f / (float)p;
                a_set((float *)s, 2.0f, 0.5f);
                *(u128 *)v = *(u128 *)(o + 0x10);
                a_pos(v, &f, data + 0x98, data + 0x9C);
                a_turn(f, self, s, 4, 5, 2);
                *(float *)(data + 0xCC) = D_0015EE6C + D_0015EE6C;
                *(float *)(data + 0xD0) = 10.0f;
                *(float *)(data + 0xD4) = 20.0f;
                a_snd(2, 0, (int)self);
                self[0x20] = 9;
            }
        }
    }
    ((unsigned char *)self)[0xA4] = 0xFF;
}
