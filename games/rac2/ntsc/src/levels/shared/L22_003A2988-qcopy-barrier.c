#include "include_asm.h"


typedef int V4_L23_418650 __attribute__((mode(TI)));
extern char D_00189ED0[];
extern float D_L23_001B36A4 __attribute__((sda));
extern void func_00282CF0(char *dst, char *src, char *arg);
extern void func_00282F20(char *dst, char *src, float arg);
extern void func_00282CC0(char *dst, char *src, char *arg);

/* Same body as func_L23_00418650, with this function's own callees. */
extern float ref_L22_003A2988_00 __attribute__((sda));
void func_L22_003A2988(char *arg0, float arg1) {
    char local[16];

    func_00282CF0(local, arg0, D_00189ED0);
    func_00282F20(local, local, arg1 + ref_L22_003A2988_00);
    func_00282CC0(local, local, D_00189ED0);
    *(V4_L23_418650 *)arg0 = *(V4_L23_418650 *)local;
}
