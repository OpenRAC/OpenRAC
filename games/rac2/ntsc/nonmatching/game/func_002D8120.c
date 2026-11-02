/* func_002D8120: 136 bytes, 28 places (us, pal and us2 each 0 of 28).
   Near miss, profile-bound.  The retail loop at 0x2D8150 is the form the compiler produces only
   with strength reduction off: it keeps the counter in $a3 counting up, tests it with
   `slt $a0,$a3,$t0` / `bnez`, advances it through a temporary (`addiu $v0,$a3,0x1` ... `daddu
   $a3,$v0,$zero`), and recomputes the byte index from scratch every pass (`subu $v1,$t0,$v0`
   then `addu $v1,$v1,$t3`), exactly the `i++`/`count - 1 - i` body below.  Under this file's
   profile (`-O2 -G0 -ffunction-sections`, config/cflags.txt) that same body is never emitted:
   the counted loop is reversed into a decrementing trip counter with a decrementing index
   pointer (128 bytes; `blez` ... `bnez $a1`, `addiu $a3,$a3,-0x1`), and the spellings that keep
   the counter counting up (`count - 1 - i`, `count - i` with `i++` first) instead
   strength-reduce the subscript into `$a3 = D_0018D0E8 + count - 1; addiu $a3,$a3,-1`
   (132 bytes, all 28 places).  With `-O2 -G0 -ffunction-sections -fno-strength-reduce` the
   loop below is instruction for instruction the retail's 13 words (same `addiu v0,a3,1`,
   `sh t1,2(a2)`, `subu v1,t0,v0`, `daddu a3,v0,zero`, `addu v1,v1,t3`, `slt`, `lbu`, `sll 2`,
   `addu`, `lhu`, `sh`, `bnez`, `addiu a2,a2,12`) and the whole tail from `lui $v1,0x1A` to
   `sh $zero,0($v1)` matches word for word; the 136-byte candidate then differs only in the
   preheader's register plan - it saves the count's %hi in $t4 for the tail (`lw $v1,0($t4)`)
   where retail rematerialises it (`lui $v1,0x1A; lw $v1,0x7C04($v1)`), and copies the record
   base's %hi into $a1 (`daddu $a1,$v1,$zero`) where retail materialises it straight into $a1
   (`lui $a1,0x26` in the `blez` delay slot) - i.e. 140 bytes with that flag.  A `for` loop with
   `count - i - 1` under the flag reaches the retail size (136) but folds the -1 into the byte
   load (`lbu $v0,-0x1($v1)`) and updates the counter in place.  So the function needs a unit
   profiled `-fno-strength-reduce` (there is precedent: src/game/003169D0-rc2.c,
   src/game/0032AF50-rc2.c, src/levels/shared/L00_00384C20.c), which is a config/cflags.txt
   change outside this task; once it exists the preheader allocation above is the only residue
   measured here. */

extern int D_001A7C04;
extern unsigned char D_0018D0E8[];
extern short D_00254EC8[][2];
extern char D_0025ADB0[];

int func_002D8120(void)
{
    int count = D_001A7C04;
    int i = 0;
    char *r = D_0025ADB0;

    while (i < count) {
        *(short *)(r + 2) = 1;
        *(short *)r = D_00254EC8[D_0018D0E8[count - 1 - i]][0];
        r += 12;
        i++;
    }
    *(short *)(D_0025ADB0 + D_001A7C04 * 12) = 0;
    return 0;
}
