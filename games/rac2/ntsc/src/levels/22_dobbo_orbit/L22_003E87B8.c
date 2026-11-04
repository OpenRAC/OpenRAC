/* func_L22_003E87B8 (120 bytes): spawns into the caller's object, paints a font-mode
   string and arms its animation, like func_L22_003D4320. func_002E5DA0 returns a value;
   declaring it void reorders the argument setup of its call. */
#include "common.h"

extern void *func_002A9640(void *, int, int);
extern char *func_002A77E0(int);
extern int func_002E5DA0(char *, int, void *, int);
extern void func_002AC728(void *, void *, int, float, float);
extern void func_0029F338(void *object);

void func_L22_003E87B8(u8 *a0, int a1, int a2) {
    u8 *s0 = a0;
    void *s3;
    s3 = func_002A9640(a0, -1, 0);
    if (s3 == 0)
        return;
    func_002E5DA0(0xC + func_002A77E0(3), 0, s0, 0xE0B);
    func_002AC728(s0, s0 + 0x10, -1, 2.0f, 13.0f);
    func_0029F338(s0);
}
