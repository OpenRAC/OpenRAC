/* G8 draft - 0 of 1 places match - last line: 13_boldan: 360 bytes, expected 372
 *
 * The body is recovered: the level's list of live nodes is walked, every node
 * of id 0x0E29 gets its 0x800 flag set, every node of id 0x0FA0 is skipped and
 * the rest are handed to func_32F1B8.  The count of nodes comes from the
 * halfword at 0x001BD5C0+0x44 and the list from the pointer that
 * 0x001A79F0 holds (+0x18C).
 *
 * Retail's shape:
 *
 *   00306F68  lui   $v1, 0x1C
 *   00306F6C  daddu $s7, $v1, $zero     ; the page of 0x001BD5C0
 *   00306F70  addiu $v1, $v1, -0x2A40   ; 0x001BD5C0
 *   00306F74  lh    $v0, 0x44($v1)     ; the count
 *   00306F78  blezl $v0, 0x30708C      ; no nodes -> skip the loop
 *   00306F7C  lw    $v1, -0x7600($gp)  ; the list pointer, in the delay slot
 *   00306F80  daddu $s3, $v1, $zero
 *
 * The 0x001A79F0 pointer is read TWICE, once $gp-relative inside the loop and
 * once absolute after it (`lui $v1,0x1A ; lw $v1,0x79F0($v1)`), and
 * 0x1AEFF0-0x7600 and 0x1A0000+0x79F0 are the same address.  The deposit
 * uses the sda name for the first access and an assembler alias of the same
 * symbol with `nosda` for the second; a second *name* for the address
 * (`D_001A79F0_2`) gives the same code.
 *
 * What is left (12 bytes) is the register allocation and the block layout:
 * retail saves $s1,$s7,$s0,$s2,$s3,$s4,$s5,$s6, keeps the list pointer in $s3
 * and the list in $s2, and re-materialises 0x001BD5C0 from the page in $s7
 * (`addiu $v1,$s7,-0x2A40`); this cc1 saves $s3,$s4,$s0,$s1,$s2,$s5,$s6,$s7,
 * keeps the list in $s1 and the constants in $s5,$s6,$s7, hoists the absolute
 * read of 0x001A79F0 to the top of the function instead of leaving it after the
 * loop, folds the copy of the list pointer into the +0x18C address, and emits
 * retail's blezl as a blez.
 *
 * Measured with `tools/check.py func_L13_00306F38 --diff` in
 * src/levels/13_boldan/L13_00306F38-g8.c (the file must be built with -G8 for
 * the $gp-relative access; the other function of that unit,
 * func_L13_0038C328, still passes 1 of 1 with this function in the unit):
 *
 *   - 340 bytes: no count test at the head of the loop;
 *   - 364 bytes: with the count test, but 0x001A79F0 read as a word instead of
 *     as the pointer it holds (one load too many);
 *   - 360 bytes: with the count test and 0x001A79F0 read as the pointer (this
 *     deposit).
 *
 * The constants are the retail words: 0x3D7A35E0 = 0.061086535f, and the two
 * integer arguments of func_32F1B8 are 0x80AAAAD2 and 0x80396464.
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef short s16;
typedef int s32;
typedef float f32;

extern u8 *D_001A79F0 __attribute__((sda));
extern u8 *D_001A79F0_v __asm__("D_001A79F0") __attribute__((nosda));
extern u8 *D_L13_001BD5C0;
extern void func_306CD0(void);
extern void func_3698B0(u8 *);
extern void func_32F1B8(u8 *, u8 *, u32, u32, f32);

void func_L13_00306F38(void) {
    u8 *s0;
    u8 *p;
    s32 *s2;
    s32 *s3;
    s32 s1;
    s32 s4;
    s32 s5;
    s32 s6;

    func_306CD0();
    s1 = 0;
    if (*(s16 *)(D_L13_001BD5C0 + 0x44) > 0) {
        s3 = (s32 *)D_001A79F0;
        s6 = 0xE29;
        s5 = 0xD54;
        s4 = 0xFA0;
        s2 = (s32 *)((u8 *)s3 + 0x18C);
        for (;;) {
            s0 = *(u8 **)s2;
            if (s0 != 0 && *(s16 *)(s0 + 0xAA) == 0xE29) {
                *(u16 *)(s0 + 0x34) |= 0x800;
            }
            if (*(s16 *)(s0 + 0xAA) == 0xD54) {
                func_3698B0(s0);
            }
            if (s0 != 0 && *(s16 *)(s0 + 0xAA) != 0xFA0) {
                p = *(u8 **)(s0 + 0x68);
                if (p != 0) {
                    func_32F1B8(s0, p + 0xC, 0x80AAAAD2, 0x80396464, 0.061086535f);
                }
            }
            s1++;
            s2++;
            if (!(s1 < *(s16 *)((u8 *)s3 + 0x44)))
                break;
        }
    }

    if (D_001A79F0_v != (u8 *)13) {
        s32 *q = *(s32 *)(D_L13_001BD5C0 + 0x30);

        if (q == 0) {
            *(u16 *)(*(u32 *)(D_L13_001BD5C0 + 0x1A0) + 0x7C) = 7;
        } else if (q == 1) {
            *(u16 *)(*(u32 *)(D_L13_001BD5C0 + 0x1A4 + 0x19C) + 0x7C) = 7;
            *(u16 *)(*(u32 *)(D_L13_001BD5C0 + 0x1A4 + 0x1A0) + 0x7C) = 7;
        }
    }
}
