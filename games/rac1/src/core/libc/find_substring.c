#include "common.h"
#include "callees.h"
#include "structs/misc.h"

/*
 * core_text object 0x116CC0-0x116D30. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * newlib's strstr.o (the SDK's libc.a). Built with Sony's 2.9-ee
 * (Makefile.sn, EE29_CORE), like libc.a.
 */


/*
 * strstr, newlib's text. Exact under 2.9-ee. (A goto-structured decode of
 * the same algorithm was reverted under 2.95.3, which duplicated the
 * `return 0` / `return searchee` tails instead of sharing them.)
 */
char *find_substring(const char *searchee, const char *lookfor) {
    if (*searchee == 0) {
        if (*lookfor)
            return (char *)0;
        return (char *)searchee;
    }

    while (*searchee) {
        unsigned int i;
        i = 0;

        while (1) {
            if (lookfor[i] == 0) {
                return (char *)searchee;
            }

            if (lookfor[i] != searchee[i]) {
                break;
            }
            i++;
        }
        searchee++;
    }

    return (char *)0;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_00116D2C);
