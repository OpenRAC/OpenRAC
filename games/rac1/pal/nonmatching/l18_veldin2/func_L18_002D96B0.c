/* NON_MATCHING func_L18_002D96B0 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: BYTES 98/1104 (91.1% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   - D_0015EE70 is gp-form AND lui-form in retail. Same-symbol mix is impossible (.extern 4 vs 2: last wins, all 
 *   so gp reads use `short D_0015EE70_s __asm__("D_0015EE70")` and lui reads use `*(float*)((char*)&D_0015EE6C + 4
 *   (MACRO_ADDR D_0015EE6C, addend +4: same address, different reloc symbol; full build might flag the addend).
 *   - `float f = gp read; if (...) { ...; f = lui read; }` fixed the bne/delay-slot load at the end.
 *   - p8: SIZE 1100/1104 (no volatile). Retail stores d70 then reloads d64 and d70 for `d64 += d70`; plain C forwa
 *   Volatile on d64/d70 in case 5 (p10, BYTES 107/1104) restores size and reload but scheduling/register numbering
 *   (d60 load/sub placement, f-register numbers). Volatile is a guess, not a verified idiom; p8 is the non-volatil
 *   - Remaining: case 3 `addiu v0,4` slot order (+0x160), case 5 tail scheduling/regs. Unresolved.
 */
extern void func_001F9C30(void *, void *, float);
extern float func_00214158(void);
extern int func_001F9908(int *);
extern float func_001FA888(int);
extern void func_L18_002D9CA8(char *moby);
extern int func_L00_00258BC8(int, int);
extern int func_001F9850(int);
extern int func_0022ED80_i(int, int, void *) __asm__("func_0022ED80");
extern float func_001F9D48(void *, void *);
extern void func_L18_002284E0(int, int);
extern void func_0020D678(void *);
extern void func_L18_002D9B00(char *moby);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_002617B0(char *, void *, void *, void *);
extern char D_0013E633[];
extern short D_L18_00162438;
extern short D_L18_00161AF0;
extern short D_L18_00161AF4;
extern short D_L18_00161AE0;
extern short D_L18_00161AE8;
extern short D_L18_00161AEC;
extern short D_L18_00161AD0;
extern short D_L18_00161AD4;
extern short D_L18_0015F718;
extern short D_0015EE70_g __asm__("D_0015EE70");

void func_L18_002D96B0(char *moby) {
    char *data = *(char **)(moby + 0x78);
    float vec[4];
    float q[4];
    float t, k, m;
    func_001F9C30(vec, moby + 0x10, -1.0f);
    qcopy(q, moby + 0x40);
    switch (((unsigned char *)moby)[0x20]) {
    case 0:
        *(float *)(data + 0x60) = *(float *)(moby + 0x18);
        *(float *)(data + 0x64) = *(float *)(moby + 0x18);
        *(int *)(data + 0x70) = 0;
        *(int *)(data + 0x68) = 0;
        *(int *)(data + 0x6C) = 0;
        *(float *)(data + 0x74) = func_00214158();
        *(int *)(data + 0x5C) |= 1;
        if (*(int *)(data + 0x88) != 0) {
            moby[0x20] = 2;
        } else {
            moby[0x20] = 1;
        }
        break;
    case 1:
    case 2:
        break;
    case 3:
        if (func_001F9908((int *)(data + 0x7C)) != 0) {
            t = (func_001FA888(*(int *)&D_L18_00162438) - *(float *)&D_L18_00161AF0) /
                (*(float *)&D_L18_00161AF4 - *(float *)&D_L18_00161AF0);
            if (t > 1.0f) {
                t = 1.0f;
            } else if (t < 0.0f) {
                t = 0.0f;
            }
            t = 1.0f - t * 0.333f;
            if (*(int *)(data + 0x84) != 0) {
                k = *(float *)&D_L18_00161AE0;
            } else {
                k = *(float *)&D_L18_00161AE8;
            }
            *(float *)(data + 0x6C) = k * t * 0.017453292f * *(float *)&D_0015EE6C;
            moby[0x20] = 4;
        }
        break;
    case 4:
        if (((unsigned char *)moby)[0x31] != 0) {
            if (func_001F9908((int *)(data + 0x8C)) != 0) {
                func_L18_002D9CA8(moby);
                *(int *)(data + 0x8C) = func_001F9850(func_L00_00258BC8(0x5A, 0x78));
            }
        }
        {
        char *g = D_0013E633 + 0xE1D;
        if (*(int *)(g + 0x2FC) == (int)moby && *(short *)(g + 0x30E) == 0) {
            *(int *)(data + 0x8C) = 0;
            moby[0x20] = 5;
            *(short *)(data + 0x92) = func_0022ED80_i(1, 0, moby);
        }
        }
        if (*(int *)(data + 0x80) != 0) {
            if (func_001F9D48(moby + 0x10, D_0013E633 + 0xE9D) < *(float *)&D_L18_00161AEC) {
                moby[0x20] = 5;
                *(short *)(data + 0x92) = func_0022ED80_i(1, 0, moby);
            }
        }
        break;
    case 5:
        if (((unsigned char *)moby)[0x31] != 0) {
            if (func_001F9908((int *)(data + 0x8C)) != 0) {
                func_L18_002D9CA8(moby);
                *(int *)(data + 0x8C) = func_001F9850(func_L00_00258BC8(0xF, 0x1E));
            }
        }
        {
            t = (func_001FA888(*(int *)&D_L18_00162438) - *(float *)&D_L18_00161AF0) /
                (*(float *)&D_L18_00161AF4 - *(float *)&D_L18_00161AF0);
            if (t > 1.0f) {
                t = 1.0f;
            } else if (t < 0.0f) {
                t = 0.0f;
            }
            t = 1.0f - t * 0.333f;
            if (*(int *)(data + 0x80) != 0 || (*(int *)(data + 0x88) != 0 && *(int *)(data + 0x84) == 0)) {
                k = *(float *)&D_0015EE70_g;
                m = 15.0f;
            } else {
                if (*(int *)(data + 0x88) == 0 && *(int *)(data + 0x84) == 0) {
                    k = *(float *)&D_L18_00161AD4;
                } else {
                    k = *(float *)&D_L18_00161AD0;
                }
                m = *(float *)((char *)&D_0015EE6C + 4);
                k = k * t;
            }
            *(volatile float *)(data + 0x70) = *(volatile float *)(data + 0x70) - k * m;
        }
        *(volatile float *)(data + 0x64) = *(volatile float *)(data + 0x64) + *(volatile float *)(data + 0x70);
        if (*(float *)(moby + 0x18) < *(float *)(data + 0x60) - 10.0f) {
            float f = *(float *)&D_0015EE70_g;
            char *g = D_0013E633 + 0xE1D;
            if (*(int *)(g + 0x2FC) == (int)moby && *(short *)(g + 0x30E) == 0) {
                if (*(float *)(g + 0x88) < *(float *)&D_L18_0015F718) {
                    if (*(int *)(g + 0x2084) != 0x77) {
                        func_L18_002284E0(0x77, 1);
                    }
                }
                f = *(float *)((char *)&D_0015EE6C + 4);
            }
            *(volatile float *)(data + 0x70) = *(volatile float *)(data + 0x70) - f * 13.7f;
            if (*(float *)(moby + 0x18) < 15.0f) {
                func_0020D678(moby);
                return;
            }
        }
        break;
    }
    func_L18_002D9B00(moby);
    func_001F9BD8(vec, vec, moby + 0x10);
    func_L00_002617B0(data + 0x20, vec, q, moby + 0x40);
}
