/* NON_MATCHING func_L00_00272BC0 -- src/overlays/shared/partupd_00272158.c
 * Best so far: BYTES 15/204 (92.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Particle update: adds vec at +0x30 (copied to a stack vec with w=0) to pos (+0x10) and +0x20, KillPart if time
 */
typedef int u128 __attribute__((mode(TI)));
extern u128 D_L00_00173F60_m __asm__("D_L00_00173F60");
extern void func_001F9BD8(void *, void *, void *);
extern int func_001F9938(void *);
extern void func_L00_002688A8(void *);
extern int func_L00_001EFFF0(void *, void *, int, void *, int);

// Particle update: moves by velocity, kills when expired, else tests collision and resets.
void func_L00_00272BC0(char *m) {
    float v[4];
    char *p = m + 0x10;
    char *q = m + 0x20;
    char *w = m + 0x30;
    v[0] = *(float *)(m + 0x30);
    v[1] = *(float *)(w + 4);
    v[2] = *(float *)(w + 8);
    v[3] = 0.0f;
    func_001F9BD8(p, p, v);
    {
        func_001F9BD8(q, q, v);
        if (func_001F9938(m + 0xA) != 0) {
            func_L00_002688A8(m);
        } else if (func_L00_001EFFF0(q, p, 0x10, *(void **)(w + 0xC), 0) != 0) {
            u128 t = D_L00_00173F60_m;
            *(short *)(m + 0xA) = 0;
            *(u128 *)p = t;
        }
    }
}
