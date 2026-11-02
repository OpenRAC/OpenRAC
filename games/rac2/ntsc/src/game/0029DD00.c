#include "common.h"

/* A unit of its own for -mno-split-addresses (config/cflags.txt): the resident bytes is then
   one assembler operand, which the assembler expands to the retail's absolute lui + load
   through the destination register, never in a delay slot. -G0 splits the address into a
   high-part register of its own; the flag would change the other bodies of the old unit. */

extern u8 D_001A7BD5;
extern u8 D_001A7B08;

int func_0029DD00(void) { if (D_001A7BD5) { if (D_001A7B08) return 1; } return 0; }
