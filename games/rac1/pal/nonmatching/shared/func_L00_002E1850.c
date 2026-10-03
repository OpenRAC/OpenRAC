/* NON_MATCHING func_L00_002E1850 -- src/overlays/shared/vendor_002E1660.c
 * Best so far: BYTES 30/776 (96.1% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002E1850 (UpdateMoby_1290): 4-state machine (spawn/wait near player/run effects/delete). Best is p4.c
 *   Remaining difference (state 2): retail loads `lh idx` before `lbu val`, then `addu base,idx` base-first for D_
 *   Would unblock: the exact source form of the two +0x454 byte-table stores (p5/p7 pointer-local variants cost re
 *   # fz5 y04
 *   Best new: p9.c BYTES 30/776 (best.c was 32): writing `(char *)(s[1] * 256 + (int)base)` (offset first) fixes t
 */
extern float D_0015EE6C MACRO_ADDR;
extern unsigned char D_0013D5CA[] NOT_SDA;
extern char D_0013E633[];
extern unsigned char D_0014171B[] NOT_SDA;
extern unsigned char D_001414F5[] NOT_SDA;
extern short D_0015EE84;
extern int D_L00_0015F6A8 MACRO_ADDR;
extern int D_L00_0015F720 MACRO_ADDR;
extern short D_L00_00160098;
extern char D_L00_001EA530[];
extern char D_L00_001EA558[];
extern char D_L00_00161C90[];
extern unsigned char D_L00_001BA960[];
extern unsigned char D_L00_0015FD48[];
extern unsigned char D_L00_001BB5C0[];
extern char D_L00_001EA580[];
extern float func_001FA748(float, float);
extern void func_L00_002D80A0(char *);
extern int func_001E9730();
extern void func_0020D678(void *);
extern void func_L00_002E1C38(char *);
extern float func_001F9D48(void *, void *);
extern float func_001F9B88(float);
extern void func_L00_00299B68(int);
extern void func_L00_00264DB8(int, int);
extern void func_L00_002618D8(int, int);
extern int func_L00_00203F20(int, int);
extern void func_L00_002512D8(int);
extern int func_0020BFC8(int, int);

/* UpdateMoby 1290: state machine that spawns, waits for the player, then deletes itself. */
void func_L00_002E1850(char *m) {
    int *s = *(int **)(m + 0x78);
    *(float *)(m + 0x48) = func_001FA748(*(float *)(m + 0x48), D_0015EE6C * 1.5707964f);
    switch (*(unsigned char *)(m + 0x20)) {
    case 0:
        func_L00_002D80A0(m);
        if (s[0] == -1) func_001E9730(D_L00_001EA530, *(short *)(m + 0xB2));
        if (D_0013D5CA[5] != 0) {
            func_001E9730(D_L00_001EA558);
            func_0020D678(m);
        } else {
            m[0x20] = 1;
            *(float *)(m + 0x18) = *(float *)(m + 0x18) + 1.0f;
        }
        break;
    case 1: {
        char *p;
        func_L00_002E1C38(m);
        p = D_0013E633 + 0xE9D;
        if (func_001F9D48(m + 0x10, p) < 3.0f) {
            p -= 0x80;
            if (func_001F9B88(*(float *)(m + 0x18) - *(float *)(p + 0x88)) < 2.0f) {
                if (*(int *)(p + 0x22A8) != 0) {
                    *(unsigned short *)(m + 0x34) |= 0x41;
                    if (s[0] != -1) func_L00_00299B68(s[0]);
                    else func_001E9730(D_L00_001EA530, *(short *)(m + 0xB2));
                    m[0x20] = 2;
                }
            }
        }
        break;
    }
    case 2:
        if (D_L00_0015F6A8 != 2) {
            func_001E9730(D_L00_00161C90);
            func_L00_00264DB8(0x232A, -1);
            D_L00_0015F720 = 0xB4;
            func_L00_002618D8(7, 1);
            if (*(unsigned short *)(D_0014171B + 0x4ED) == 0) func_L00_00203F20(0x2328, 0x34);
            if (s[1] != -1) {
                char *base = (char *)*(int *)&D_L00_00160098;
                char *o = (char *)(s[1] * 256 + (int)base);
                char *o2;
                int u;
                D_L00_001BA960[0x454 + *(short *)(o + 0xB2)] = *(unsigned char *)(o + 0xB0) + 2;
                o2 = (char *)(s[1] * 256 + (int)base);
                u = *(unsigned char *)(o2 + 0xB0);
                if (u == 0xFF || (D_L00_0015FD48[u & 0xFF] != 0xFF
                    && (D_0014171B + 0xAA35)[(u & 0xFF) + (*(int *)&D_0015EE84 << 4)] == 0xFF)) {
                    D_L00_001BB5C0[0x454 + *(short *)(o2 + 0xB2)] = u + 2;
                }
            } else {
                func_001E9730(D_L00_001EA580, *(short *)(m + 0xB2));
            }
            if (*(unsigned char *)(m + 0xB0) != 0xFF) func_L00_002512D8(*(unsigned char *)(m + 0xB0));
            *(int *)(D_001414F5 + 0x1B) = 7;
            m[0x20] = 3;
        }
        break;
    case 3:
        func_0020BFC8(0, -1);
        func_0020D678(m);
        break;
    }
}
