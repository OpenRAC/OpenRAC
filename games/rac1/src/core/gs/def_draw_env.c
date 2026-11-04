#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "core/cdvd/read.h"

/*
 * core_text object 0x122200-0x1224B0. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Sony's libgraph: graph005.o (sceGszbufaddr, sceGszbufaddr) and
 * graph006.o (sceGsSetDefDrawEnv, sce_gs_set_def_draw_env), back to back. Built with
 * Sony's 2.9-ee (Makefile.sn, EE29_CORE).
 */

/*
 * sceGszbufaddr-style size: fbw = (w + 63) / 64 pages across; fbh =
 * (h + 63) / 64 for a psm with bit 1 set, else (h + 31) / 32; returned as
 * a short, doubled unless the first 8 bytes of sceGsGetGParam()
 * (sceGsGetGParam) masked with 0x0000FFFF0000FFFF equal 1 (interlace mode
 * 1, field mode 0).
 *
 * The old decode's "clamps" are plain signed divisions by 64 and 32:
 * gcc's own expansion of `x / 64` is the `addiu; slt -1; movn` sequence
 * retail has, where the hand-written clamps always gave slti/movz (192
 * against 200). The GParam test has to be the explicit 64-bit mask (two
 * short field compares fold shorter, SIZE 196). Exact under 2.9-ee with
 * the `== 1` arm first; 2.95.3's best is 8/200, a prologue-order residual.
 */
short sceGszbufaddr(short psm, short w, short h) {
    unsigned long *g = sceGsGetGParam();
    int fbw, fbh;

    fbw = (w + 63) / 64;
    if (psm & 2) {
        fbh = (h + 63) / 64;
    } else {
        fbh = (h + 31) / 32;
    }
    if ((*g & 0x0000FFFF0000FFFFUL) == 1) {
        return fbw * fbh;
    }
    return fbw * fbh * 2;
}

/* Sony's sceGsDrawEnv1 (libgraph.h): eight (value, GS-register-address)
 * pairs, all 64-bit ("long" on this ABI). Offsets confirmed against the
 * SET_FRAME/SET_ZBUF/SET_XYOFFSET/SET_SCISSOR/SET_TEST bit layouts and
 * the FRAME_1/ZBUF_1/XYOFFSET_1/SCISSOR_1/PRMODECONT/COLCLAMP/DTHE/
 * TEST_1 register ids in eestruct.h. */
typedef struct {
    unsigned long frame1, frame1addr;
    unsigned long zbuf1, zbuf1addr;
    unsigned long xyoffset1, xyoffset1addr;
    unsigned long scissor1, scissor1addr;
    unsigned long prmodecont, prmodecontaddr;
    unsigned long colclamp, colclampaddr;
    unsigned long dthe, dtheaddr;
    unsigned long test1, test1addr;
} DrawEnv1_1222C8;

extern short sceGszbufaddr(short psm, short w, short h); /* sceGszbufaddr */

/* sceGsSetDefDrawEnv (libgraph.h): fills in a default drawing
 * environment for a psm/w/h framebuffer. zbuf1's page count reuses
 * sceGszbufaddr (the same page-count helper FRAME's own fbw comes from)
 * on the color psm/w/h, since the Z buffer is laid out right after the
 * color buffer; ztest==0 disables Z (mask writes, ZTE/ZTST forced on).
 * prmodecont/colclamp are read-modify-write: only their AC/CLAMP bit is
 * set, whatever else the caller's memory already held is left alone. */
int sce_gs_set_def_draw_env(DrawEnv1_1222C8 *draw, short psm, short w, short h, short ztest, short zpsm) {
    short fbw = ((w + 63) >> 6) & 0x3F;
    short zbp;

    draw->frame1addr = 0x4C;
    draw->frame1 = ((unsigned long)fbw << 16) | ((unsigned long)(psm & 0xF) << 24);
    draw->zbuf1addr = 0x4E;
    if (ztest == 0) {
        zbp = sceGszbufaddr(psm, w, h);
        draw->zbuf1 = (unsigned long)zbp | ((unsigned long)(zpsm & 0xF) << 24) | (1UL << 32);
    } else {
        zbp = sceGszbufaddr(psm, w, h);
        draw->zbuf1 = (unsigned long)zbp | ((unsigned long)(zpsm & 0xF) << 24);
    }
    draw->xyoffset1addr = 0x18;
    {
        long ofx = 2048L - (w >> 1);
        long ofy = 2048L - (h >> 1);
        draw->xyoffset1 = (unsigned long)(ofx << 4) | ((unsigned long)(ofy << 4) << 32);
    }
    draw->scissor1addr = 0x40;
    draw->scissor1 = ((unsigned long)(w - 1) << 16) | ((unsigned long)(h - 1) << 48);
    draw->prmodecontaddr = 0x1A;
    draw->prmodecont |= 1;
    draw->colclampaddr = 0x46;
    draw->colclamp |= 1;
    draw->dtheaddr = 0x45;
    if (psm & 2) {
        draw->dthe |= 1;
    } else {
        draw->dthe &= ~1UL;
    }
    draw->test1addr = 0x47;
    if (ztest != 0) {
        draw->test1 = ((unsigned long)(ztest & 3) << 17) | 0x10000;
    } else {
        draw->test1 = 0x30000;
    }
    __asm__ __volatile__("sync");
    return 8;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_001224AC);
