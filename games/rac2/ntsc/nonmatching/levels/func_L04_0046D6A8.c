#include "include_asm.h"

typedef unsigned char u8;
typedef unsigned int u32;
typedef int s32;
typedef float f32;

/* Historical screen corrected: packed R5900 integer/vector operations alone
 * do not establish handwritten assembly or prevent a C implementation.
 * This entry has no complete retained C attempt. Its observed vector setup
 * and transforms still need true object extents, call contracts and full
 * carrying-placement qualification. The earlier MMI exclusion is withdrawn.
 * No new match or assembly classification is claimed.
 */

INCLUDE_ASM("asm/levels", func_L04_0046D6A8);
