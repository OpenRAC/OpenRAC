/* NON_MATCHING func_L15_002A47B8 -- src/overlays/shared/vendor_00298BB8.c
 * Best so far: SIZE ours 240 / retail 244, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Releases a moby's slots: writes byte (m[0xB0]+2) into two level tables at +0x454 indexed by m[0xB2] (second on
 *   Best candidate p6.c/p7.c: 240 of 244 bytes. Matching so far: 0x454 in the sb displacement (struct with pad[0x4
 *   Unblock: a source form that makes gcc keep a real register copy of the byte (I tried unsigned char var, int c 
 */
typedef struct { char pad[0x454]; unsigned char a[1]; } S454;
extern S454 D_L15_001BAEE0;
extern unsigned char D_L15_0015FD08[];
extern S454 D_L15_001BBB40;
extern short D_0015EE84;
extern void func_L00_0028EBF0(int);
extern unsigned char D_0014171B_2d[][16] __asm__("D_0014171B");
extern char D_0013E633[];

/* Clear a moby's link and release its slots. */
void func_L15_002A47B8(char *m) {
    int a;
    unsigned char b;
    int c;
    *(int *)(m + 0x94) = 0;
    D_L15_001BAEE0.a[*(short *)(m + 0xB2)] = *(unsigned char *)(m + 0xB0) + 2;
    a = *(unsigned char *)(m + 0xB0);
    if (a != 0xFF) {
        if (D_L15_0015FD08[(unsigned char)a] == 0xFF) goto skip;
        if ((&D_0014171B_2d[0xAA35 / 16][0xAA35 % 16])[*(int *)&D_0015EE84 * 16 + (unsigned char)a] != 0xFF) goto skip;
    }
    D_L15_001BBB40.a[*(short *)(m + 0xB2)] = a + 2;
skip:
    b = *(unsigned char *)(m + 0xBC);
    if (b != 0xFF) {
        char *o;
        c = b;
        o = D_0013E633 + 0x1D + c * 0x70;
        if (*(char **)(o + 0x88) == m && *(unsigned char *)(o + 0x74) != 0)
            func_L00_0028EBF0(c);
    }
    *(unsigned char *)(m + 0xBC) = 0xFF;
    *(unsigned char *)(m + 0x20) = 2;
}
