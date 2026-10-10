/* NON_MATCHING func_L10_002EAA08 -- src/overlays/l10_orxon/vendor_002E30F8.c
 * Best so far: SIZE ours 812 / retail 836, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Gate moby (class 1421) update on level 10: a four-state switch; states 0 and 1 test a per-bit table (word at D
 *   Best candidate p3.c/p4.c: 812 bytes against retail 836, prologue and state dispatch now match. Differences lef
 *   Unblock: settle the D_0015EE84 access form (gp or lui) per use, then the table index expression order.
 */
extern int D_L10_001BAC60[];
extern char D_0013DE4B[];
extern char D_L10_001BB9C0[];
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern float func_001FA748(float, float);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_001F9908(void *);
extern int func_00215570(void *arg0, int arg1);
extern void func_L10_002F6E10(int i);

/* Gate (moby class 1421) update on level 10: opens, closes and reports by a per-bit table. */
void func_L10_002EAA08(char *m) {
    char *mv;
    char *p7;
    char *p20;
    char *q;
    int st;
    int sv;
    int uv;
    int t;
    int r;
    float f;

    st = *(unsigned char *)(m + 0x20);
    mv = *(char **)(m + 0x78);

    switch (st) {
    case 0:
        *(unsigned char *)(m + 0x30) = 0xFF;
        if (*(unsigned char *)(D_0013E633 + 0x2EC1) != 0) goto b50;
        sv = (short)*(unsigned short *)(m + 0xB2);
        uv = *(unsigned short *)(m + 0xB2);
        if (((unsigned char *)D_L10_001BB9C0)[sv + 0x454] != 0) goto b50;
        p20 = D_0014171B + 0xAB75;
        t = (*(int *)(p20 + ((sv >> 5) << 2) + (D_0015EE84_m << 8)) >> (uv & 0x1F)) & 1;
        if (t != 0) goto b50;
        *(float *)(mv + 4) = *(float *)(m + 0x44);
        m[0x20] = 1;
        break;
    case 1:
        p7 = D_0013E633 + 0xE1D;
        if (*(unsigned char *)(p7 + 0x20A4) != 0) goto b50;
        sv = (short)*(unsigned short *)(m + 0xB2);
        uv = *(unsigned short *)(m + 0xB2);
        if (((unsigned char *)D_L10_001BB9C0)[sv + 0x454] != 0) goto b50;
        p20 = D_0014171B + 0xAB75;
        t = (*(int *)(p20 + ((sv >> 5) << 2) + (D_0015EE84_m << 8)) >> (uv & 0x1F)) & 1;
        if (t != 0) goto b50;
        q = p7 + 0x80;
        r = func_00215570(q, *(int *)(mv + 0xC));
        if ((r != 0 || ((unsigned char *)D_0013DE4B)[9] != 0) && *(int *)mv > 0 && *(int *)(mv + 0x10) > 0) {
            *(unsigned int *)(p20 + ((sv >> 5) << 2) + (D_0015EE84_m << 8)) |= st << (uv & 0x1F);
            *(unsigned int *)((char *)D_L10_001BAC60 + ((sv >> 5) << 2)) |= st << (uv & 0x1F);
            if (func_00215570(q, *(int *)(mv + 0xC)) != 0) {
                func_L10_002F6E10(*(int *)(mv + 0x10));
            }
            *(int *)(mv + 8) = func_001FA898_r(func_001F9878(90.0f));
            m[0x20] = 2;
        }
        break;
    case 2:
        f = func_001F9878(90.0f);
        *(float *)(m + 0x44) = func_001FA748(*(float *)(m + 0x44), -1.3962634f / f);
        if (func_001F9908(mv + 8)) m[0x20] = 3;
        break;
    case 3:
        if (*(unsigned char *)(D_0013E633 + 0x2EC1) != 0) break;
        sv = (short)*(unsigned short *)(m + 0xB2);
        uv = *(unsigned short *)(m + 0xB2);
        if (((unsigned char *)D_L10_001BB9C0)[sv + 0x454] != 0) break;
        p20 = D_0014171B + 0xAB75;
        t = (*(int *)(p20 + ((sv >> 5) << 2) + (D_0015EE84_m << 8)) >> (uv & 0x1F)) & 1;
        if (t != 0) break;
        f = func_001FA748(*(float *)(m + 0x44), 1.3962634f);
        *(float *)(m + 0x44) = f;
        m[0x20] = 0;
        break;
    }
    return;
b50:
    f = func_001FA748(*(float *)(m + 0x44), -1.3962634f);
    *(float *)(m + 0x44) = f;
    m[0x20] = 3;
}
