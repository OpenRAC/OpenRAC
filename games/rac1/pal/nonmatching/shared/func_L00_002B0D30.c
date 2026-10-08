/* NON_MATCHING func_L00_002B0D30 -- src/overlays/shared/vendor_002AB910.c
 * Best so far: SIZE ours 540 / retail 548, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Debris moby update: applies gravity, then state 0 bounces off the ground (reflect, random spin) and state 1 fa
 *   Tried const globals, switch vs if, struct-typed moby/data (in_struct stores), operand order: same bytes. Likel
 *   Wave lb1 p10: ported Lombyte's FUN_L00_002afa48 (p8.c: u128 old + qcopy, else-if/&& chain, structs with aligne
 *   Only diff: retail's gp-rel global loads (-0x57E8/E4 before the two rnd calls, -0x57BC in state 1) are schedule
 *   Would unblock: a way to declare a 4-byte global gp-relative at -G2 that does not alias stores (none found); no
 */
extern void func_001F9BD8(void *, void *, void *);
extern float func_001FA748(float, float);
extern int func_001F9938(void *);
extern int func_L00_001F10E0(float, void *, int, void *);
extern float func_001F9C78(void *, void *);
extern void func_L00_001FF610(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern float func_L00_00258C80(float lo, float hi);
extern void func_0020D678(void *);
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_L00_00173F80[];
extern short D_L00_00161518_g __asm__("D_L00_00161518");
extern short D_L00_0016151C_g __asm__("D_L00_0016151C");
extern short D_L00_00161540_g __asm__("D_L00_00161540");
extern short D_L00_00161544_g __asm__("D_L00_00161544");

/* Falling debris moby: bounces off the ground, then fades out and deletes. */
void func_L00_002B0D30(char *m) {
    char *d = *(char **)(m + 0x78);
    float save[4];
    qcopy(save, m + 0x10);
    *(float *)(d + 0x18) -= D_0015EE70 * 20.0f;
    func_001F9BD8(m + 0x10, m + 0x10, d + 0x10);
    *(float *)(m + 0x40) = func_001FA748(*(float *)(m + 0x40), *(float *)(d + 0x20));
    *(float *)(m + 0x44) = func_001FA748(*(float *)(m + 0x44), *(float *)(d + 0x24));
    *(float *)(m + 0x48) = func_001FA748(*(float *)(m + 0x48), *(float *)(d + 0x28));
    if (*(unsigned char *)(m + 0x20) == 0) {
        if (*(float *)(m + 0x18) < *(float *)(d + 4) - 3.0f) {
            m[0x20] = 1;
            return;
        }
        if (func_001F9938(d + 0xE) == 0) return;
        if (*(short *)(d + 0xC) == 0) return;
        if (func_L00_001F10E0(*(float *)(d + 8), m + 0x10, 2, 0) == 0) return;
        if (func_001F9C78(d + 0x10, D_L00_00173F80) < 0.0f) {
            qcopy(m + 0x10, D_L00_00173F80 - 4);
            *(unsigned short *)(d + 0xC) -= 1;
            func_L00_001FF610(d + 0x10, d + 0x10, D_L00_00173F80);
            func_001F9C30(d + 0x10, d + 0x10, 0.75f);
            *(float *)(d + 0x20) = 0.0f;
            *(float *)(d + 0x24) = func_L00_00258C80(*(float *)&D_L00_00161518_g, *(float *)&D_L00_0016151C_g) * 0.0174532925f * D_0015EE6C;
            *(float *)(d + 0x28) = func_L00_00258C80(*(float *)&D_L00_00161518_g, *(float *)&D_L00_0016151C_g) * 0.0174532925f * D_0015EE6C;
        }
    } else if (*(unsigned char *)(m + 0x20) == 1) {
        *(float *)(m + 0x2C) *= *(float *)&D_L00_00161540_g;
        if ((unsigned char)m[0x23] < (*(int *)&D_L00_00161544_g)) {
            func_0020D678(m);
        } else {
            m[0x23] = m[0x23] - (*(int *)&D_L00_00161544_g);
        }
    }
}
