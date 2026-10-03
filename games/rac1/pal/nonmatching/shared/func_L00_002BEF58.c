/* NON_MATCHING func_L00_002BEF58 -- src/overlays/shared/vendor_002BA7C8.c
 * Best so far: SIZE ours 1068 / retail 1064, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Everything matches except the float allocation and scheduling around the shared gp-rel float at -0x551C
 *   (D_L00_001617E4): retail loads it into the bc1f delay slot ($f0, reused on the else path and reloaded only
 *   inside the row==11 block), then loads the row height into $f1; ours puts it in $f3 (or $f1) and the row height
 *   in $f0, so the add/sub/mov and the stores of the two quads shift by register number. The stack-store order
 *   (sw 0x40..0x4C, sd 0x70) needs the rotated source order used in p8.c. Learned: `long` (not `long long`)
 *   gives sd; locals for yh/x0/x1 across the call cost extra callee-saved floats, so recompute y - [E0] inline.
 *   Unblock: find source wording where the delta value is first read before the dot-product branch without
 *   overlapping the dot result ($f0), e.g. a different arm structure; not found.
 */
extern char D_0013E633[];
extern float D_0015EE60 MACRO_ADDR;
extern char D_L00_001DC320[];
extern char D_L00_001DC3E0[];
extern char D_L00_001DC360[];
extern char D_L00_001DC4A0[];
extern short D_L00_00161860;
extern short D_L00_0016185C;
extern short D_L00_0016183C;
extern short D_L00_00161840;
extern short D_L00_00161844;
extern short D_L00_00161848;
extern short D_L00_0016184C;
extern short D_L00_00161854;
extern short D_L00_00161858;
extern short D_L00_00161868;
extern short D_L00_00161864;
extern short D_L00_0016186C;
extern short D_L00_001617EC;
extern short D_L00_001617E8;
extern short D_L00_001617F8;
extern short D_L00_001617F0;
extern short D_L00_001617F4;
extern short D_L00_001617E4;
extern short D_L00_001617E0;
extern void func_L00_002BF380(void *, int, int, int, float, float, float, int);
extern void func_L00_002BF958(void *);
extern void func_L00_002BFB98(void *);
extern int func_001F4868(int);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9CA0(void *, void *, void *);
extern float func_001F9C78(void *a, void *b);
extern void func_L00_001FD1D8(void *, void *, int);

/* Draws the scrolling 10x20 grid of quads, colouring and offsetting each row. */
void func_L00_002BEF58(void *m) {
    float quad[36];
    float e0[4], e1[4], e2[4], e3[4];
    float t, t2, x0, x1, y;
    int c, row, j;
    t = *(float *)&D_L00_0016185C - *(float *)&D_L00_00161860 * D_0015EE60;
    *(float *)&D_L00_0016185C = t;
    if (t <= -8.0f) {
        *(float *)&D_L00_0016185C = t + 8.0f;
    }
    func_L00_002BF380(D_L00_001DC320, *(int *)&D_L00_0016183C, *(int *)&D_L00_00161840, 12,
        *(float *)&D_L00_00161844, *(float *)&D_L00_00161848, *(float *)&D_L00_0016184C, *(int *)&D_L00_00161854);
    func_L00_002BF380(D_L00_001DC3E0, *(int *)&D_L00_0016183C, *(int *)&D_L00_00161840, 12,
        *(float *)&D_L00_00161844, *(float *)&D_L00_00161848, *(float *)&D_L00_0016184C, *(int *)&D_L00_00161858);
    func_L00_002BF958(m);
    func_L00_002BFB98(m);
    if (*(unsigned char *)(D_0013E633 + 2) != 0) {
        c = *(int *)&D_L00_00161868;
    } else {
        c = *(int *)&D_L00_00161864;
    }
    *(long *)((char *)quad + 0x78) = func_001F4868(*(int *)&D_L00_0016186C);
    y = *(float *)&D_L00_001617E8;
    *(long *)((char *)quad + 0x80) = 0xFF9000000260L;
    *(long *)((char *)quad + 0x88) = 0x8000000048L;
    *(int *)((char *)quad + 0x40) = c;
    *(long *)((char *)quad + 0x70) = 0;
    *(int *)((char *)quad + 0x4C) = c;
    *(int *)((char *)quad + 0x48) = c;
    *(int *)((char *)quad + 0x44) = c;
    *(float *)&D_L00_001617E8 = y - *(float *)&D_L00_001617EC * D_0015EE60;
    if (*(float *)&D_L00_001617E8 <= -8.0f) {
        *(float *)&D_L00_001617E8 += 8.0f;
    }
    t2 = *(float *)&D_L00_001617F8 * D_0015EE60;
    *(float *)&D_L00_001617F0 -= t2;
    if (*(float *)&D_L00_001617F0 <= -7.0f) {
        *(float *)&D_L00_001617F0 += 7.0f;
    }
    *(float *)&D_L00_001617F4 += t2;
    if (*(float *)&D_L00_001617F4 >= 5.0f) {
        *(float *)&D_L00_001617F4 -= 5.0f;
    }
    for (row = 2; row < 12; row++) {
        x0 = *(float *)&D_L00_001617F0;
        x1 = *(float *)&D_L00_001617F4;
        for (j = 0; j < 20; j++) {
            qcopy(quad, D_L00_001DC4A0 + row * 0x140 + j * 16);
            qcopy(quad + 4, D_L00_001DC4A0 + row * 0x140 + ((j + 1) % 20) * 16);
            qcopy(quad + 8, D_L00_001DC360 + row * 0x140 + j * 16);
            qcopy(quad + 12, D_L00_001DC360 + row * 0x140 + ((j + 1) % 20) * 16);
            func_001F9BF0(e0, quad, quad + 4);
            func_001F9BF0(e1, quad + 8, quad + 4);
            func_001F9CA0(e2, e1, e0);
            func_001F9BF0(e3, quad + 4, D_L00_00166EC0);
            if (func_001F9C78(e2, e3) >= 0.0f) {
                x0 += *(float *)&D_L00_001617E4;
                x1 += *(float *)&D_L00_001617E4;
            } else {
                if (row == 11) {
                    int k = *(int *)&D_L00_00161864 & 0xFFFFFF;
                    *(int *)((char *)quad + 0x40) = k;
                    *(int *)((char *)quad + 0x44) = k;
                }
                t = x0 + *(float *)&D_L00_001617E4;
                *(float *)((char *)quad + 0x64) = x0;
                *(float *)((char *)quad + 0x54) = x0;
                *(float *)((char *)quad + 0x50) = y;
                *(float *)((char *)quad + 0x5C) = t;
                *(float *)((char *)quad + 0x68) = y - *(float *)&D_L00_001617E0;
                *(float *)((char *)quad + 0x60) = y - *(float *)&D_L00_001617E0;
                *(float *)((char *)quad + 0x6C) = t;
                *(float *)((char *)quad + 0x58) = y;
                x0 = t;
                func_L00_001FD1D8(quad, 0, 0);
                t = x1 + *(float *)&D_L00_001617E4;
                *(float *)((char *)quad + 0x64) = x1;
                *(float *)((char *)quad + 0x54) = x1;
                *(float *)((char *)quad + 0x50) = y;
                *(float *)((char *)quad + 0x58) = y;
                *(float *)((char *)quad + 0x68) = y - *(float *)&D_L00_001617E0;
                *(float *)((char *)quad + 0x5C) = t;
                *(float *)((char *)quad + 0x60) = y - *(float *)&D_L00_001617E0;
                *(float *)((char *)quad + 0x6C) = t;
                x1 = t;
                func_L00_001FD1D8(quad, 0, 0);
            }
        }
        y += *(float *)&D_L00_001617E0;
    }
}
