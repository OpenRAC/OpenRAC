#include "common.h"
#include "structs.h"

/*
 * core_text object 0x114518-0x1154C0. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * newlib (the SDK's libc.a), nine members back to back: lseekr.o
 * (_lseek_r), makebuf.o (__smakebuf), mallocr.o (malloc_extend_top,
 * _malloc_r), mbtowc_r.o (_mbtowc_r), memchr.o, memcmp.o, memcpy.o,
 * memmove.o and memset.o. Built with Sony's 2.9-ee (Makefile.sn,
 * EE29_CORE), like libc.a; _mbtowc_r is newlib's own text.
 */

/* Declarations in scope here before the split. */
extern long func_00116F68(int arg0, int arg1, int arg2);
extern int D_0015ED10;
extern void *D_0012F86C NOT_SDA;
extern int func_001162B8(void *arg0, void *arg1, void *arg2);
extern int func_00116320(void *arg0, void *arg1, void *arg2);
extern long func_001163A0(void *arg0, void *arg1, void *arg2);
extern void func_00116408(void *arg0);
extern void func_00113968(void);
extern void func_00114438(void *, void *);
extern char D_00152470[];

int func_00114518(int *errOut, void *a, void *b, void *c) {
    int r;
    D_0015ED10 = 0;
    r = func_00119108(a, b, c);
    if (r == -1 && D_0015ED10 != 0) {
        *errOut = D_0015ED10;
    }
    return r;
}

/* newlib makebuf.c __smakebuf(fp): allocate fp's stdio buffer (or fall
 * back to unbuffered). __SNBF -> use the 1-byte fp->_nbuf and return.
 * Otherwise fstat the descriptor to size the buffer and pick __SOPT
 * (regular file, default seek) vs __SNPT; BUFSIZ/blksize are both 0x400
 * here (HAVE_BLKSIZE is off in this build). malloc failure falls back
 * to unbuffered too; success sets _cleanup_r, __SMBF, and __SLBF when
 * the descriptor is a tty. */
extern int func_00114000(void *data, int file, void *st);  /* _fstat_r */
extern void *func_00114920(void *data, unsigned int size); /* _malloc_r */
extern int func_001191C0(int);                              /* isatty */
extern void func_00113AC8(void);                             /* _cleanup_r */
extern long func_001163A0(void *, void *, void *);           /* __sseek */

typedef struct {
    short st_dev;
    short st_ino;
    int st_mode;
    unsigned short st_nlink;
    unsigned short st_uid;
    unsigned short st_gid;
    short st_rdev;
    long st_size;
    long st_atime;
    long st_spare1;
    long st_mtime;
    long st_spare2;
    long st_ctime;
    long st_spare3;
    long st_blksize;
    long st_blocks;
    long st_spare4[2];
} Stat_114578;

typedef struct {
    void *_p;
    char pad_04[0xC - 0x4];
    short _flags;
    short _file;
    void *_bf_base;
    unsigned int _bf_size;
    char pad_18[0x28 - 0x18];
    void *_seek;
    char pad_2C[0x43 - 0x2C];
    unsigned char _nbuf[1];
    char pad_44[0x4C - 0x44];
    unsigned int _blksize;
    char pad_50[0x54 - 0x50];
    void *_data;
} File_114578;

void func_00114578(File_114578 *fp) {
    unsigned int size, couldbetty;
    void *p;
    Stat_114578 st;

    if (fp->_flags & 0x2) {
        fp->_bf_base = fp->_p = fp->_nbuf;
        fp->_bf_size = 1;
        return;
    }
    if (fp->_file < 0 || func_00114000(fp->_data, fp->_file, &st) < 0) {
        couldbetty = 0;
        size = 0x400;
        fp->_flags |= 0x800;
    } else {
        couldbetty = (st.st_mode & 0xF000) == 0x2000;
        size = 0x400;
        if ((st.st_mode & 0xF000) == 0x8000 && fp->_seek == (void *)func_001163A0) {
            fp->_flags |= 0x400;
            fp->_blksize = 0x400;
        } else {
            fp->_flags |= 0x800;
        }
    }
    if ((p = func_00114920(fp->_data, size)) == 0) {
        fp->_flags |= 0x2;
        fp->_bf_base = fp->_p = fp->_nbuf;
        fp->_bf_size = 1;
    } else {
        *(void (**)(void))((char *)fp->_data + 0x3C) = func_00113AC8;
        fp->_flags |= 0x80;
        fp->_bf_base = fp->_p = p;
        fp->_bf_size = size;
        if (couldbetty && func_001191C0(fp->_file))
            fp->_flags |= 0x1;
    }
}

typedef struct MallocChunk {
    unsigned int prev_size;
    unsigned int size;
    struct MallocChunk *fd;
    struct MallocChunk *bk;
} MallocChunk;

extern struct MallocChunk *D_0012F888[3]; /* malloc_av_ bins; av_[2] is `top` */
extern unsigned long D_0012FC98; /* malloc_top_pad (unsigned long) */
extern char *D_0012FCA0;         /* malloc_sbrk_base */
extern int D_0012FCB8;           /* current_mallinfo.arena (sbrked_mem) */
extern unsigned long D_0012FCA8; /* malloc_max_sbrked_mem */
extern unsigned long D_0012FCB0; /* malloc_max_total_mem */

extern void *func_001161E8(void *reent_ptr, unsigned int size); /* _sbrk_r */
extern int func_00113B70(void *reent_ptr, void *mem);           /* _free_r */

#define TOP (D_0012F888[2])
#define INITIAL_TOP ((MallocChunk *)D_0012F888)

/* newlib malloc_extend_top: grows the arena's top chunk with sbrk for an
 * nb-byte request no free chunk could satisfy, both when sbrk returns
 * memory right after the old top and when it doesn't (then the old top is
 * fenced off and freed). TOP has to be an array element: through a cast
 * pointer the compiler keeps base+8 in a register instead of reloading
 * the address after each sbrk call as retail does. */
void func_001146C8(void *reent_ptr, unsigned int nb) {
    char *brk;
    unsigned int front_misalign;
    unsigned int correction;
    char *new_brk;
    unsigned int top_size;

    MallocChunk *old_top = TOP;
    unsigned int old_top_size = old_top->size & ~3u;
    char *old_end = (char *)old_top + old_top_size;

    unsigned int sbrk_size = nb + D_0012FC98 + 0x10;
    unsigned long pagesz = 0x1000;

    if (D_0012FCA0 != (char *)-1)
        sbrk_size = (sbrk_size + (pagesz - 1)) & ~(pagesz - 1);

    brk = (char *)func_001161E8(reent_ptr, sbrk_size);

    if (brk == (char *)-1 || (brk < old_end && old_top != INITIAL_TOP))
        return;

    D_0012FCB8 += sbrk_size;

    if (brk == old_end) {
        top_size = sbrk_size + old_top_size;
        TOP->size = top_size | 1;
    } else {
        if (D_0012FCA0 == (char *)-1)
            D_0012FCA0 = brk;
        else
            D_0012FCB8 += brk - old_end;

        front_misalign = (unsigned int)(brk + 8) & 0xF;
        if (front_misalign > 0) {
            correction = 0x10 - front_misalign;
            brk += correction;
        } else
            correction = 0;

        correction += pagesz - ((unsigned int)(brk + sbrk_size) & (pagesz - 1));

        new_brk = (char *)func_001161E8(reent_ptr, correction);
        if (new_brk == (char *)-1)
            return;

        D_0012FCB8 += correction;

        TOP = (MallocChunk *)brk;
        top_size = new_brk - brk + correction;
        TOP->size = top_size | 1;

        if (old_top != INITIAL_TOP) {
            if (old_top_size < 0x10) {
                TOP->size = 1;
                return;
            }
            old_top_size = (old_top_size - 12) & ~0xFu;
            old_top->size = (old_top->size & 1) | old_top_size;
            *(unsigned int *)((char *)old_top + old_top_size + 4) = 5;
            *(unsigned int *)((char *)old_top + old_top_size + 8) = 5;
            if (old_top_size >= 0x10)
                func_00113B70(reent_ptr, (char *)old_top + 8);
        }
    }

    if (D_0012FCB8 > D_0012FCA8)
        D_0012FCA8 = D_0012FCB8;
    if ((unsigned long)D_0012FCB8 > D_0012FCB0)
        D_0012FCB0 = D_0012FCB8;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00114920);

/*
 * _mbtowc_r, newlib's text (1999, before multibyte locales): a byte
 * becomes the wide char; NULL s returns 0, n == 0 returns -1.
 *
 * Exact under 2.9-ee. The same logic nested the other way was 56 bytes
 * against 60 under 2.95.3 and stayed a stub: retail's `bnel n,0` puts
 * the byte load of the branch target in its likely slot, which 2.9-ee
 * does from newlib's own two early returns.
 */
int func_00115098(void *r, int *pwc, const char *s, unsigned int n, int *state) {
    int dummy;
    unsigned char *t = (unsigned char *)s;

    if (pwc == 0)
        pwc = &dummy;

    if (s != 0 && n == 0)
        return -1;

    if (s == 0)
        return 0;  /* not state-dependent */

    *pwc = (int)*t;
    return (*t != '\0');
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_001150D4);

INCLUDE_ASM("asm/nonmatchings/core_text", func_001151B4);

INCLUDE_ASM("asm/nonmatchings/core_text", func_00115248);

INCLUDE_ASM("asm/nonmatchings/core_text", func_001152F8);

INCLUDE_ASM("asm/nonmatchings/core_text", func_001153FC);

INCLUDE_ASM("asm/nonmatchings/core_text", func_001154BC);
