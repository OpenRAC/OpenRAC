#include "include_asm.h"

extern int *D_001B22A8;
extern void func_002FC560(unsigned int, unsigned long);
extern void func_002FC518(int);

/* Adapted from RAC1 framebuf.c, draw_centered_capture_background (GPL-3.0).
 * Append a centered background as 32-pixel sprite strips. */
void func_00285398(int w, int h) {
    int *tag;
    long *q;
    long *c;
    int n;
    int i;
    int zero;
    int step;
    int x;
    int y;
    long lo;
    long hi;

    n = w / 32;
    func_002FC560(0x42, 0x800000004AL);
    *(int *)((char *)D_001B22A8 + 0) = (n + 5) | 0x10000000;
    *(int *)((char *)D_001B22A8 + 4) = 0;
    *(int *)((char *)D_001B22A8 + 8) = 0;
    *(int *)((char *)D_001B22A8 + 12) = (n + 5) | 0x50000000;
    tag = D_001B22A8;
    q = (long *)(tag + 4);
    D_001B22A8 = tag + 4;
    zero = 0;
    q[zero] = 0x1000000000000001L;
    q[1] = 0xE;
    q[2] = 0x32003;
    q[3] = 0x47;
    q[4] = 0x2400000000000001L;
    q[5] = 0x10;
    q[6] = 0x146;
    q[7] = 0x80008080L;
    q[8] = (long)(n | 0x8000) | 0x2400000000000000L;
    q[9] = 0x44;
    i = zero;
    if (n > zero) {
        lo = (long)(0x8000 - h * 8) << 16;
        x = 0x8000 + -(w * 8);
        c = (long *)((char *)tag + 0x60);
        y = 0x8200 + -(w * 8);
        hi = (long)(h * 8 + 0x7FF0) << 16;
        do {
            step = 0x200;
            *c++ = x | lo;
            *c++ = y | hi;
            y += step;
            x += step;
            i++;
        } while (i < n);
    }
    D_001B22A8 = D_001B22A8 + (n * 4 + 20);
}


/* Adapted from RAC1 framebuf.c, draw_framebuffer_rect (GPL-3.0).
 * Append an alpha-blended rectangle in the centered 12.4 coordinate space. */
void func_00285548(int x0, int y0, int x1, int y1, int w, int h, unsigned int rgba) {
    long *q;
    int ax, ay, bx, by;
    int hw, hh;

    func_002FC518(0x13000000);
    func_002FC560(0x42, 0x64);
    *(int *)((char *)D_001B22A8 + 0) = 0x10000006;
    *(int *)((char *)D_001B22A8 + 4) = 0;
    *(int *)((char *)D_001B22A8 + 8) = 0;
    *(int *)((char *)D_001B22A8 + 12) = 0x50000006;
    ax = (x0 << 4) + 0x8000;
    ay = (y0 << 4) + 0x8000;
    bx = (x1 << 4) + 0x8000;
    by = (y1 << 4) + 0x8000;
    hw = w << 3;
    hh = h << 3;
    D_001B22A8 += 4;
    q = (long *)D_001B22A8;
    *(long *)((char *)q + 0) = 0x1000000000000001L;
    *(long *)((char *)q + 8) = 0xE;
    *(long *)((char *)q + 16) = 0x33003;
    *(long *)((char *)q + 24) = 0x47;
    *(long *)((char *)q + 32) = 0x2400000000000001L;
    *(long *)((char *)q + 40) = 0x10;
    *(long *)((char *)q + 48) = 0x106;
    *(long *)((char *)q + 56) = rgba;
    *(long *)((char *)q + 64) = 0x2400000000008001L;
    *(long *)((char *)q + 72) = 0x44;
    {
        long X1 = bx - hw;
        long X0 = ax - hw;
        long Y0 = ay - hh;
        long Y1 = by - hh;

        *(long *)((char *)q + 80) = X0 | (Y0 << 16);
        *(long *)((char *)q + 88) = X1 | (Y1 << 16);
    }
    D_001B22A8 += 0x18;
    func_002FC560(0x42, 0x8000000044L);
    func_002FC518(0x13000000);
}

