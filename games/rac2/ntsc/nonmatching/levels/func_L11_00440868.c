/* GPT-6.1 Sol low, 28 October 2026. Current pinned compiler, -O2 -G0
 * -ffunction-sections: 264/264 bytes, 0/1 places. Only three non-relocation
 * words differ at +0x30, +0x34 and +0x3c: list loaded into v1 instead of v0,
 * beqz instead of beql, cursor copied from v1 instead of v0. Everything after
 * +0x40 matches. Explicit forward class_b3a and next labels fix handler order;
 * incomplete-array sda alias fixes base rematerialization. Shared failure label
 * and returning the null list give identical bytes. Shared success return makes
 * 256 bytes and moves handlers outside the loop. Next: list/result lifetime.
 * Declarations required are included below. Not integrated.
 */
typedef int s32;
typedef unsigned int u32;
typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;

extern u8 *D_L11_001FC420[];
extern u8 *D_L11_001B359C_base[] __asm__("D_L11_001B359C") __attribute__((sda));

extern void func_L11_003F2F38(u8 *);

extern void func_L11_00439950(u8 *);

/* Finds the class record the identifier names: the first live record of the state's
   class list whose identifier matches, and hands it to the class's handler. */
u8 *func_L11_00440868(u8 *obj, s32 id) {
    u8 *state = *(u8 **)(obj + 0x68);
    s32 index = *(s32 *)(state + 0xC0);
    u16 *cursor;
    u8 *record;
    u8 *list;

    if (index == 0xFF) {
        return 0;
    }
    list = D_L11_001FC420[index];
    if (list == 0) {
        return 0;
    }
    cursor = (u16 *)list;
    do {
        u16 value = *cursor;
        record = D_L11_001B359C_base[0] + ((value & 0x7FFF) << 8);

        if (record != 0 && record[0x20] != 0xFE && record[0x20] != 0xFD &&
            *(s16 *)(record + 0xAA) == id && record[0x20] == 1) {
            *(u32 *)(record + 0xB8) = (u32)obj;
            if (id == 0xB3A) goto class_b3a;
            if (id == 0xD4A) {
                func_L11_00439950(record);
                *(s32 *)(state + 0x118) += 1;
                return record;
            }
            goto next;
class_b3a:
            {
                func_L11_003F2F38(record);
                *(s32 *)(state + 0x11C) += 1;
                return record;
            }

        }
next:
        ;
    } while (*(s16 *)cursor++ >= 0);
    return 0;
}
