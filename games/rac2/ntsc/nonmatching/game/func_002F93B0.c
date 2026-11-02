/* GPT-6.1 Sol high, 29 October 2026. Game -O2 -G0 -ffunction-sections.
 * Best retained assigned-unit check: 948/964 bytes, 0/28 USA v1.01 places.
 * Eight exploratory checker invocations this task. Other forms reached 956 bytes but
 * differed more throughout navigation; this one retains its instruction shape.
 * Corrected the older draft: retail always writes handle = -1 after movement,
 * including when it was already -1. The mode field is declared volatile because
 * retail reloads it at 0x002F95E8 and 0x002F9604 with no intervening call/store.
 * Scroll remains ordinary: the mode read already invalidates its earlier value.
 * Separate state/input lifetimes and a named 24-byte stride/kind base recover
 * the saved-register allocation. Animation temporaries, repeated held-button
 * guard, purchase pointer/ID allocation and branch scheduling still differ.
 * Requires the original unit's common.h, D_00138180 declaration, the settings
 * object D_001A79F8 with its MACRO_ADDR declaration, D_0021F240 declaration,
 * and int func_002E5CF8(int, int, void *). Other declarations are below.
 * No linked build run. Next: source structure for the repeated purchase guard.
 */
extern int D_001B2240, D_001B223C;
extern int D_001B2244 __attribute__((sda));
extern int D_00139688[];
extern void func_0028B6C8(int, unsigned int);
extern int func_002F6510(void);
extern void func_002F9198(void);

extern char D_00239BA0[];
extern unsigned char D_00139568[];

typedef struct { int id, kind; int fields[4]; } ShopRow_93;
typedef struct {
    char pad0[0x28]; char *moby; char pad2C[0x24];
    volatile int mode; char pad54[0x18]; int scroll;
    char pad70[8]; int handle; char pad7C[4]; int selected;
    int busy; int animated; char pad8C[4]; int animation;
    char pad94[0xAC]; ShopRow_93 rows[64]; int count;
} ShopInput_93;
extern ShopInput_93 F93State __asm__("D_0021F240");
typedef struct { char pad[0x1C4]; int buttons; } Input_93;
void func_002F93B0(void)
{
    ShopInput_93 *state = &F93State;
    int moved, cost;
    int *item;
    char *record;
    if (state->busy) return;
    {
        Input_93 *input = (Input_93 *)D_00138180;
        if ((input->buttons & 0x2000) ||
                (D_001B2240 >= 16 && D_001B2240 % 10 == 1)) {
            if (state->scroll < 57) {
                moved = 0;
                if (state->selected < state->count - 1) {
                    ++state->selected;
                    moved = 1;
                }
                if (moved) {
                    {
                        int animation = 3;
                        if (state->animated) {
                            if (state->mode != 2) animation = 1;
                            state->animation = animation;
                        }
                    }
                    state->scroll += 56;
                    func_002E5CF8(1, 17, state->moby);
                    if (state->handle != -1) {
                        if (state->mode == 0)
                            func_0028B6C8(state->handle, 0);
                    }
                    F93State.handle = -1;
                }
            }
        }
    }
    {
        Input_93 *input = (Input_93 *)D_00138180;
        ShopInput_93 *state;
        if ((input->buttons & 0x8000) ||
                (D_001B223C >= 16 && D_001B223C % 10 == 1)) {
            state = &F93State;
            if (state->scroll >= -56) {
                moved = 0;
                if (state->selected > 0) {
                    --state->selected;
                    moved = 1;
                }
                if (moved) {
                    {
                        int animation = 3;
                        if (state->animated) {
                            if (state->mode != 2) animation = 1;
                            state->animation = animation;
                        }
                    }
                    state->scroll -= 56;
                    func_002E5CF8(1, 17, state->moby);
                    if (state->handle != -1) {
                        if (state->mode == 0)
                            func_0028B6C8(state->handle, 0);
                    }
                    F93State.handle = -1;
                }
            }
        }
    }
    {
        Input_93 *input = (Input_93 *)D_00138180;
        ShopInput_93 *state;
        if (input->buttons & 0x40) {
            int held = D_001B2244 & 0x40;
            if (!held) {
                state = &F93State;
                if (state->mode == 3) {
                    if (held) goto finish;
                    if (state->mode != 3) goto finish;
                    if (*(int *)((char *)state + 0x144 + state->selected * 12) == 0) goto finish;
                }
                if (state->mode == 0) {
                    int stride = sizeof(ShopRow_93);
                    int *kinds = &state->rows[0].kind;
                    item = (int *)((char *)kinds + state->selected * stride);
                    if (*item == 0) {
                        cost = func_002F6510();
                        if ((*(int *)&D_001A79F8) >= cost) goto activate;
                    }
                    item = (int *)((char *)kinds + state->selected * stride);
                    if (*item != 1) goto rejected;
                    record = D_00239BA0 + D_00139568[state->rows[state->selected].id] * 224;
                    if (D_00139688[state->rows[state->selected].id] >= *(unsigned short *)(record + 0x8E)) goto rejected;
                    if ((*(int *)&D_001A79F8) < *(unsigned short *)(record + 0x88)) goto rejected;
                }
activate:
                state = &F93State;
                if (state->animated) state->animation = 1;
                state->busy = 1;
                func_002E5CF8(0, 17, state->moby);
                goto finish;
rejected:
                func_002E5CF8(2, 17, *(char **)(((char *)&D_0021F240) + 0x28));
            }
        }
    }
finish:
    if (((Input_93 *)D_00138180)->buttons & 0x10) func_002F9198();
}
