#include "common.h"
#include "callees.h"
#include "structs/misc.h"

/*
 * core_text object 0x125218-0x1252A0. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Sony's libvib, libvib.o (sceVibGetProfile; sceVibSetActParam was
 * dead-stripped). Built with Sony's 2.9-ee (Makefile.sn, EE29_CORE),
 * like the prebuilt libvib.a.
 */

typedef struct {
    unsigned long a : 14;
    unsigned long b : 2;
    unsigned long c : 8;
    unsigned long d : 8;
} Mode125218;

extern int sceDbcReceiveData_v(int, Mode125218, int *, int) __asm__("func_00124A70");

/* Builds a mode word from bitfields and runs the sceDbcReceiveData round
   trip. The fields are 64-bit, so each insert is a doubleword and/or,
   and the struct is passed by value. `if (r >= 0) return out;` keeps
   retail's two returns; an `r = out` spelling becomes slt/movz. */
int sce_vib_get_profile(int arg0, int arg1) {
    Mode125218 m;
    int out;
    int r;

    m.a = 2;
    m.b = 2;
    m.c = 3;
    m.d = 1;
    r = sceDbcReceiveData_v(arg0, m, &out, arg1);
    if (r >= 0) {
        return out;
    }
    return r;
}

LINKER_REMNANT("asm/remnants/core_text", func_00125298);
