#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "core/kernel/kprintf.h"
#include "asm_functions/executable.h"
#include "core/cdvd/read.h"
#include "core/libc/atoi_calloc.h"

/*
 * core_text object 0x121D18-0x122140. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Sony's libgraph, graph003.o (checkModelVersion, _GetGsDxDyOffset,
 * sceGsSetDefDispEnv). Built with Sony's 2.9-ee (Makefile.sn,
 * EE29_CORE), like the prebuilt libgraph.a.
 */

extern void scePrintf();

extern int sceOpen(void *path, int mode);

extern void sceClose(int fd);

extern char romver_path[];

/* Read a small text file a byte at a time into a 0x100-byte stack buffer,
 * stopping at a NUL or the buffer limit, then strtol() the tail starting
 * 9 bytes before the end and compare it against a build number. Same
 * open/read/close triplet as scf_read_romver above. */
int checkModelVersion(void) {
    int fd;
    char buf[0x100];
    char *p;
    unsigned int n;

    fd = sceOpen(romver_path, 1);
    if (fd < 0) {
        return -1;
    }
    n = 0;
    p = buf;
    while (n < 0x100) {
        sceRead(fd, p, 1);
        if (*p++ == 0) {
            break;
        }
        n++;
    }
    sceClose(fd);
    return 0x1315670 < libc_atoi(&buf[n - 9]);
}

ASM_FUNC("asm/handwritten/core_text", func_00121DB8);

typedef struct {
    short interMode;
    short outMode;
    short ffMode;
    short version;
} GsGParam;

typedef struct {
    unsigned long pmode;
    unsigned long smode2;
    unsigned long dispfb;
    unsigned long display;
    unsigned long bgcolor;
} GsDispEnv;

extern char gs_def_disp_env_mode_error_msg[];

void sceGsDefDispEnv(GsDispEnv *disp, short psm, short w, short h, short dx, short dy) {
    GsGParam *gp;
    int dx1, dy1, dx2, dy2;
    int mode;
    short omode;
    unsigned short imode;

    gp = sceGsGetGParam();
    if (gp->outMode != 2 && gp->outMode != 3 && checkModelVersion() != 0) {
        _GetGsDxDyOffset(gp->outMode, &dx1, &dy1, &dx2, &dy2);
    } else {
        dy2 = 0;
        dx2 = 0;
        dy1 = 0;
        dx1 = 0;
    }
    imode = gp->interMode;
    disp->pmode = 0x66;
    if (gp->interMode != 0) {
        if (gp->ffMode != 0) {
            disp->smode2 = 3;
        } else {
            disp->smode2 = 1;
        }
    } else {
        disp->smode2 = 2;
    }
    disp->dispfb = ((unsigned long)(psm & 0xF) << 15) | ((unsigned long)(((w + 63) >> 6) & 0x3F) << 9);
    omode = gp->outMode;
    if (omode == 2) {
        if ((short)imode == 1) {
            int magh = (w + 2559) / w;
            unsigned long dxo = (unsigned long)dx1 + 636;
            disp->display =
                ((((unsigned long)(magh - 1) << 23) |
                  (((dx * magh + dxo) & 0xFFF) | ((unsigned long)(gp->ffMode ? (h << 1) - 1 : h - 1) << 44))) |
                 (((unsigned long)(dy + dy1 + 50) & 0xFFF) << 12)) |
                ((unsigned long)(magh * w - 1) << 32);
        } else {
            int magh = (w + 2559) / w;
            unsigned long dxo = (unsigned long)dx1 + 636;
            magh = (w + 2559) / w;
            disp->display = ((((unsigned long)((w + 2559) / w - 1) << 23) |
                              (((dx * ((w + 2559) / w) + dxo) & 0xFFF) | ((unsigned long)(h - 1) << 44))) |
                             (((unsigned long)(dy + dy1 + 25) & 0xFFF) << 12)) |
                            ((unsigned long)((w + 2559) / w * w - 1) << 32);
        }
    } else if (omode == 3) {
        if ((short)imode == 1) {
            int magh = (w + 2559) / w;
            unsigned long dxo = (unsigned long)dx1 + 656;
            disp->display =
                ((((unsigned long)(magh - 1) << 23) |
                  (((dx * magh + dxo) & 0xFFF) | ((unsigned long)(gp->ffMode ? (h << 1) - 1 : h - 1) << 44))) |
                 (((unsigned long)(dy + dy1 + 72) & 0xFFF) << 12)) |
                ((unsigned long)(magh * w - 1) << 32);
        } else {
            int magh = (w + 2559) / w;
            unsigned long dxo = (unsigned long)dx1 + 656;
            magh = (w + 2559) / w;
            disp->display = ((((unsigned long)((w + 2559) / w - 1) << 23) |
                              (((dx * ((w + 2559) / w) + dxo) & 0xFFF) | ((unsigned long)(h - 1) << 44))) |
                             (((unsigned long)(dy + dy1 + 36) & 0xFFF) << 12)) |
                            ((unsigned long)((w + 2559) / w * w - 1) << 32);
        }
    } else if (omode == 0x50) {
        unsigned long dxo = (unsigned long)(dx * 2) + 232;
        disp->display =
            ((((unsigned long)(h - 1) << 44) | ((((720 - w) / 2 * 2 + (unsigned long)dx1 + dxo) & 0xFFF) | 0x800000)) |
             (((unsigned long)(dy + dy1 + 35) & 0xFFF) << 12)) |
            ((unsigned long)(w * 2 - 1) << 32);
    } else {
        scePrintf(gs_def_disp_env_mode_error_msg);
    }
    disp->bgcolor = 0;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_0012213C);
