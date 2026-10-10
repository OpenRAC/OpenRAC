/* NON_MATCHING func_L14_00306BE0 -- src/overlays/l14_oltanis/vendor_002FF358.c
 * Best so far: BYTES 8/736 (98.9% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern char D_0013E633_x[] __asm__("D_0013E633");
extern unsigned char D_0014171B_x[] __asm__("D_0014171B");
extern int D_0015EE84_x __asm__("D_0015EE84") MACRO_ADDR;
extern int D_L14_0015F6A8_x __asm__("D_L14_0015F6A8") MACRO_ADDR;
extern int D_L14_0015F6B0 MACRO_ADDR;
extern float D_L14_00174668;
extern unsigned char D_0013D5DD[];
extern short D_L14_001622B8;
extern short D_L14_001622BC;
extern short D_L14_001622C0;
extern void func_L14_00306EC0(char *moby);
extern void func_L14_00306FA0(char *p);
extern float func_00214358(void *, int, float);
extern float func_001FA748(float, float);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001F9D48(void *, void *);
extern float func_001F9B88(float);
extern void func_L00_002618D8(int, int);
extern void func_L00_00299B68(int);
extern void func_L00_002512D8(int);
extern int func_0020BFC8(int, int);
extern void func_L00_00264DB8(int arg0, int arg1);
extern int func_L00_00203F20(int a, int b);
extern void func_0020D678(void *);
extern float func_00214D28(float *, float, float);

/* Spinning collectible: hovers over the ground until the hero touches it, then plays the pickup and fades out. */
void func_L14_00306BE0(char *m) {
    char *d = *(char **)(m + 0x78);
    float v[4];
    switch (((unsigned char *)m)[0x20]) {
    case 0:
        if ((D_0014171B_x + 0xAA35)[((unsigned char *)m)[0xB0] + (D_0015EE84_x << 4)] == 0xFF) {
            func_0020D678(m);
            return;
        }
        func_L14_00306EC0(m);
        func_00214358(m + 0x10, 0, 0.5f);
        *(int *)(m + 0x40) = 0;
        *(float *)(m + 0x44) = -1.5707964f;
        *(float *)(m + 0x18) = *(float *)&D_L14_001622B8 + D_L14_00174668;
        qcopy(d, m + 0x10);
        m[0x20] = 1;
        break;
    case 1:
        *(float *)(m + 0x48) = func_001FA748(*(float *)(m + 0x48), *(float *)&D_L14_001622BC);
        qcopy(m + 0x10, d);
        func_001F9C30(v, m + 0xD0, *(float *)&D_L14_001622C0);
        func_001F9BD8(m + 0x10, m + 0x10, v);
        func_L14_00306FA0(m);
        if (D_L14_0015F6B0 % 10 != 0) return;
        {
            char *h = D_0013E633_x + 0xE9D;
            if (!(func_001F9D48(m + 0x10, h) < 1.0f)) return;
            if (func_001F9B88(*(float *)(m + 0x18) - *(float *)(h + 8)) < 2.0f) {
                if (D_0013D5DD[0] != 0) {
                    *(int *)(d + 0x18) = 1;
                } else {
                    *(int *)(d + 0x18) = 0;
                }
                func_L00_002618D8(0x15, 1);
                func_L00_00299B68(6);
                m[0x20] = 2;
            }
        }
        break;
    case 2:
        if (D_L14_0015F6A8_x != 2) {
            if ((D_0014171B_x + 0xAA35)[((unsigned char *)m)[0xB0] + (D_0015EE84_x << 4)] != 0xFF) {
                func_L00_002512D8(((unsigned char *)m)[0xB0]);
                func_0020BFC8(0, -1);
            }
            if (*(int *)(d + 0x18) != 0) {
                func_L00_00264DB8(0x53E5, -1);
            } else {
                func_L00_00264DB8(0x36B2, -1);
            }
            func_L00_00203F20(0x36B0, 0x7A);
            func_0020D678(m);
        } else {
            *(unsigned short *)(m + 0x34) |= 0x41;
        }
        break;
    case 3:
        func_00214D28((float *)(m + 0x2C), 0.0f, *(float *)(*(char **)(m + 0x24) + 0x24) * 0.02f);
        if (*(float *)(m + 0x2C) == 0.0f) func_0020D678(m);
        break;
    }
}
