#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x116CC0-0x116D30. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * newlib's strstr.o (the SDK's libc.a). Built with Sony's 2.9-ee
 * (Makefile.sn, EE29_CORE), like libc.a.
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

/*
 * strstr, newlib's text. Exact under 2.9-ee. (A goto-structured decode of
 * the same algorithm was reverted under 2.95.3, which duplicated the
 * `return 0` / `return searchee` tails instead of sharing them.)
 */
char *find_substring(const char *searchee, const char *lookfor) {
    if (*searchee == 0)
    {
        if (*lookfor)
            return (char *) 0;
        return (char *) searchee;
    }

    while (*searchee)
    {
        unsigned int i;
        i = 0;

        while (1)
        {
            if (lookfor[i] == 0)
            {
                return (char *) searchee;
            }

            if (lookfor[i] != searchee[i])
            {
                break;
            }
            i++;
        }
        searchee++;
    }

    return (char *) 0;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00116D2C);
