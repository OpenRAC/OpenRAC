#include "include_asm.h"

extern void func_0027C540(long);
extern void func_0027C660(void);
extern void func_0029C450(void);
extern void func_0027C540(long arg0);
extern void func_0027C540(long param_1);
void func_0027C540(long);
void func_0027C660(void);

/*
 * Function: func_002D2EE0
 *
 * Disassembly summary:
 * - Saves return address on stack
 * - Calls func_0027C540(0) (with zero argument)
 * - Calls func_0029C7A0()
 * - Calls func_0027C660()
 * - Restores return address
 * - Returns 0
 *
 * Notes:
 * - The function follows standard MIPS ABI prologue/epilogue.
 * - Delay slots are filled with NOP or argument setup as seen in disassembly.
 * - No local variables used.
 * - All external function addresses come from the assigned externals list.
 */

/* External functions declared with exact aliases from task.externals */

long long func_US2_002CE878(void)
{
    func_0027C540(0LL);
    func_0029C450();
    func_0027C660();
    return 0LL;
}

