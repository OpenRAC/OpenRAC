#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x118A40-0x119328. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's EE kernel library (libkernl), three members back to back:
 * klib.o (the syscall stubs, CreateSema ... _InitTLB), glue.o (newlib's
 * system-call glue: write, read, close, lseek, sbrk, fstat, VSync ...)
 * and cache.o (SyncDCache, InvalidDCache). Built with Sony's 2.9-ee
 * (Makefile.sn, EE29_CORE), like the prebuilt libkernl.a.
 */

/* Declarations in scope here before the split. */
extern long strtol(int arg0, int arg1, int arg2);
extern int SbrkErrno;
extern void *_impure_ptr NOT_SDA;
extern int __sread(void *arg0, void *arg1, void *arg2);
extern int __swrite(void *arg0, void *arg1, void *arg2);
extern long __sseek(void *arg0, void *arg1, void *arg2);
extern void __sclose(void *arg0);
extern void libc_fflush(void);
extern void _fwalk(void *, void *);
extern char locale_lconv[];
extern int libc_glue_read();
extern long func_00116108_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_00116108");
extern long func_001188C8_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_001188C8");
extern long func_00114518_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_00114518");
extern int _close_r(int *errOut, int arg1);
extern int __sfvwrite(int, void *);
extern void __sinit(void *);
extern void _vfprintf_r(void *, void *, int, int);
extern int libc_glue_write();
extern int _lseek_r(int *errOut, void *a, void *b, void *c);

ASM_FUNC("asm/handwritten/core_text", func_00118A40);

ASM_FUNC("asm/handwritten/core_text", func_00118A50);

ASM_FUNC("asm/handwritten/core_text", func_00118A60);

ASM_FUNC("asm/handwritten/core_text", func_00118A70);

ASM_FUNC("asm/handwritten/core_text", func_00118A80);

ASM_FUNC("asm/handwritten/core_text", func_00118A90);

ASM_FUNC("asm/handwritten/core_text", func_00118AA0);

ASM_FUNC("asm/handwritten/core_text", func_00118AB0);

ASM_FUNC("asm/handwritten/core_text", func_00118AC0);

ASM_FUNC("asm/handwritten/core_text", func_00118AD0);

ASM_FUNC("asm/handwritten/core_text", func_00118AE0);

ASM_FUNC("asm/handwritten/core_text", func_00118AF0);

ASM_FUNC("asm/handwritten/core_text", func_00118B00);

ASM_FUNC("asm/handwritten/core_text", func_00118B10);

ASM_FUNC("asm/handwritten/core_text", func_00118B20);

ASM_FUNC("asm/handwritten/core_text", func_00118B30);

ASM_FUNC("asm/handwritten/core_text", func_00118B40);

ASM_FUNC("asm/handwritten/core_text", func_00118B50);

ASM_FUNC("asm/handwritten/core_text", func_00118B60);

ASM_FUNC("asm/handwritten/core_text", func_00118B70);

ASM_FUNC("asm/handwritten/core_text", func_00118B80);

ASM_FUNC("asm/handwritten/core_text", func_00118B90);

ASM_FUNC("asm/handwritten/core_text", func_00118BA0);

ASM_FUNC("asm/handwritten/core_text", func_00118BB0);

ASM_FUNC("asm/handwritten/core_text", func_00118BC0);

ASM_FUNC("asm/handwritten/core_text", func_00118BD0);

ASM_FUNC("asm/handwritten/core_text", func_00118BE0);

ASM_FUNC("asm/handwritten/core_text", func_00118BF0);

ASM_FUNC("asm/handwritten/core_text", func_00118C00);

ASM_FUNC("asm/handwritten/core_text", func_00118C10);

ASM_FUNC("asm/handwritten/core_text", func_00118C20);

ASM_FUNC("asm/handwritten/core_text", func_00118C30);

ASM_FUNC("asm/handwritten/core_text", func_00118C40);

ASM_FUNC("asm/handwritten/core_text", func_00118C50);

ASM_FUNC("asm/handwritten/core_text", func_00118C60);

ASM_FUNC("asm/handwritten/core_text", func_00118C70);

ASM_FUNC("asm/handwritten/core_text", func_00118C80);

ASM_FUNC("asm/handwritten/core_text", func_00118C90);

ASM_FUNC("asm/handwritten/core_text", func_00118CA0);

ASM_FUNC("asm/handwritten/core_text", func_00118CB0);

ASM_FUNC("asm/handwritten/core_text", func_00118CC0);

ASM_FUNC("asm/handwritten/core_text", func_00118CD0);

ASM_FUNC("asm/handwritten/core_text", func_00118CE0);

ASM_FUNC("asm/handwritten/core_text", func_00118CF0);

ASM_FUNC("asm/handwritten/core_text", func_00118D00);

ASM_FUNC("asm/handwritten/core_text", func_00118D10);

ASM_FUNC("asm/handwritten/core_text", func_00118D20);

ASM_FUNC("asm/handwritten/core_text", func_00118D30);

ASM_FUNC("asm/handwritten/core_text", func_00118D40);

ASM_FUNC("asm/handwritten/core_text", func_00118D50);

ASM_FUNC("asm/handwritten/core_text", func_00118D60);

ASM_FUNC("asm/handwritten/core_text", func_00118D70);

ASM_FUNC("asm/handwritten/core_text", func_00118D80);

ASM_FUNC("asm/handwritten/core_text", func_00118D90);

ASM_FUNC("asm/handwritten/core_text", func_00118DA0);

ASM_FUNC("asm/handwritten/core_text", func_00118DB0);

ASM_FUNC("asm/handwritten/core_text", func_00118DC0);

ASM_FUNC("asm/handwritten/core_text", func_00118DD0);

ASM_FUNC("asm/handwritten/core_text", func_00118DE0);

ASM_FUNC("asm/handwritten/core_text", func_00118DF0);

ASM_FUNC("asm/handwritten/core_text", func_00118E00);

ASM_FUNC("asm/handwritten/core_text", func_00118E10);

ASM_FUNC("asm/handwritten/core_text", func_00118E20);

ASM_FUNC("asm/handwritten/core_text", func_00118E30);

ASM_FUNC("asm/handwritten/core_text", func_00118E40);

ASM_FUNC("asm/handwritten/core_text", func_00118E50);

ASM_FUNC("asm/handwritten/core_text", func_00118E60);

ASM_FUNC("asm/handwritten/core_text", func_00118E70);

ASM_FUNC("asm/handwritten/core_text", func_00118E80);

ASM_FUNC("asm/handwritten/core_text", func_00118E90);

ASM_FUNC("asm/handwritten/core_text", func_00118EA0);

ASM_FUNC("asm/handwritten/core_text", func_00118EB0);

extern int CoreGlobalWord NOT_SDA;

void tty_reset_init_flag(void) {
    CoreGlobalWord = 0;
}

extern int disable_interrupts(void); /* DIntr */
extern void enable_interrupts(void); /* EIntr */

/* VSync (glue.o): clear INTC_STAT's VBlank-start bit (guarded by
 * DIntr/EIntr so the read-modify-write can't race the interrupt handler),
 * then poll INTC_STAT until that bit is set again; repeats the
 * clear/guard once more before returning so a caller's own poll starts
 * from a clean flag. */
void wait_vblank_start(void) {
    int old;

    old = disable_interrupts();
    *(volatile unsigned int *)0x1000F000 = 4;
    __asm__ __volatile__("sync");
    if (old) {
        enable_interrupts();
    }
    while ((*(volatile unsigned int *)0x1000F000 & 4) == 0) {
    }

    old = disable_interrupts();
    *(volatile unsigned int *)0x1000F000 = 4;
    __asm__ __volatile__("sync");
    if (old) {
        enable_interrupts();
    }
}

extern void SetVSyncFlag(unsigned int *, long *); /* SetVSyncFlag */
extern int disable_interrupts(void);                    /* DIntr */
extern void enable_interrupts(void);                    /* EIntr */

/* VSync2 (glue.o): like VSync, but also arms an asynchronous flag via
 * SetVSyncFlag before the guarded INTC_STAT clear, and the poll loop
 * exits on either the raw hardware bit or that flag; returns the 64-bit
 * value SetVSyncFlag filled in. */
long wait_vblank_csr(void) {
    volatile unsigned int flag;
    volatile long result;
    int old;

    flag = 0;
    SetVSyncFlag((unsigned int *)&flag, (long *)&result);

    old = disable_interrupts();
    *(volatile unsigned int *)0x1000F000 = 4;
    __asm__ __volatile__("sync");
    if (old) {
        enable_interrupts();
    }
    while ((*(volatile unsigned int *)0x1000F000 & 4) == 0 && flag == 0) {
    }

    old = disable_interrupts();
    *(volatile unsigned int *)0x1000F000 = 4;
    __asm__ __volatile__("sync");
    if (old) {
        enable_interrupts();
    }
    return result;
}

/* func_00119CC8 takes no argument (it sets $a0 itself before its first
   call); the file's `extern int func_00119CC8(int)` below is wrong. */
extern int sceTtyInit_v(void) __asm__("func_00119CC8");
extern int sceTtyWrite(int, int);

/* Deci2 tty write for fd 1/2: open the tty channel on first use
   (func_00119CC8), then write. Calling the opener with no argument is
   what leaves retail's `jal; nop`: the `a0 = arg1` in the bnez slot is
   the write's argument, stolen from the branch target. */
/* func_00119CC8 takes no argument; passing one puts a redundant move in
   the jal slot and swaps $s0/$s1. */
int libc_glue_write(int fd, int buf, int len) {
    if (fd == 1 || fd == 2) {
        if (CoreGlobalWord == 0) {
            if (sceTtyInit_v() == 0) {
                return -1;
            }
            CoreGlobalWord = 1;
        }
        return sceTtyWrite(buf, len);
    }
    return -1;
}

extern int sceTtyInit(int);
extern int sceTtyRead(int, int);

extern int sceTtyInit_v(void) __asm__("func_00119CC8");

/* As func_00119008: func_00119CC8 is called with no argument. */
int libc_glue_read(int arg0, int arg1, int arg2) {
    if (arg0 == 0) {
        if (CoreGlobalWord == 0) {
            if (sceTtyInit_v() == 0) {
                return -1;
            }
            CoreGlobalWord = 1;
        }
        return sceTtyRead(arg1, arg2);
    }
    return -1;
}

/* Takes an argument its callers pass (e.g. func_00112468) and ignores
   it; an unused parameter costs no codegen. */
int libc_glue_close(int arg0) {
    return -1;
}

/* Takes the three arguments its callers pass (e.g. func_00114518) and
   ignores them; unused parameters cost no codegen. */
int libc_glue_lseek(void *a, void *b, void *c) {
    return -1;
}

ASM_FUNC("asm/handwritten/core_text", func_00119110);

int libc_glue_isatty(void) {
    return 1;
}

int libc_glue_fstat(void *arg0, void *arg1) {
    char *p = (char *)arg1;
    *(long *)(p + 0x48) = 0;
    *(int *)(p + 0x4) = 0x2000;
    return 0;
}

ASM_FUNC("asm/handwritten/core_text", func_001191E0);

ASM_FUNC("asm/handwritten/core_text", func_00119288);

LINKER_REMNANT("asm/remnants/core_text", func_00119308);
