/* func_00294D80 (QueueGsTextureUpload) - 196 bytes, 28 places, boot + all 27 levels.
 *
 * Near miss: the candidate compiles to the retail's 49 instructions with the
 * same shape, but the register assignment differs, so no place matches.
 * `tools/check.py func_00294D80` reports "instruction at +0x0 differs" at
 * every place.
 *
 * What the retail body does (all measured from the retail bytes):
 *   - six arguments: a0, a1 (int), a2, a3 (u32), and the 5th and 6th in $t0
 *     and $t1.  This compiler's ABI passes the 5th argument in $t0 and the
 *     6th in $t1 (measured: a six-argument function reads them there, not on
 *     the stack).
 *   - shift = a0 - 6, clamped at 0 with the `x = (x >= 0) ? x : 0` shape
 *     (retail: `addiu $v1,$zero,-1 ; slt $v1,$v1,$v0 ; movz $v0,$zero,$v1`).
 *   - the returned 64-bit value is an OR of six terms, in this order:
 *       (1 << shift) << 14
 *       (u64)(arg6 >> 8)
 *       ((u64)a0 << 26) | 0x01300000
 *       (u64)a1 << 30
 *       ((u64)(arg5 >> 8) << 37) | 0x400000000
 *       0x8000000000000000
 *     The two constants are 0x01300000 (retail `lui $v1,0x130`, i.e. a 32-bit
 *     li) and 0x400000000 (retail `ori $a1,$zero,0x8000 ; dsll $a1,$a1,19`,
 *     the assembler's dli expansion).  The mask 0x8000000000000000 comes out
 *     as `addiu $v1,$zero,-1 ; dsll32 $v1,$v1,31`.
 *   - the queue is 16-byte records at 0x1B9340 (g_texUploadQueue) and the
 *     count at 0x1B15FC (g_texUploadCount); the count store is the bare
 *     symbol form (`lui $at,0x1B ; sw $a1,0x15FC($at)`), which needs the
 *     `sda` attribute on the extern in this -G0 unit.
 *   - the record's address is materialised twice, once as index+base
 *     (`addu $v0,$a0,$v1`) and once as base+index (`addu $v1,$v1,$a0`); the
 *     array-subscript spelling reproduces both.
 *
 * What still differs: only the register allocation.  The retail puts the
 * constant 1 in $t2 and reloads the 64-bit copy of a0 into $a0
 * (`daddu $a0,$t3,$zero`); this compiler puts the constant 1 in $a0 and the
 * copy in $t2, which shifts every later register.  The instruction sequence,
 * the sizes, the branch and the store order are the retail's.
 *
 * Tried and measured:
 *   - the nested spelling `value |= ((u64)a0 << 26) | 0x01300000;` flattens
 *     the group and merges the constant into the chain: the retail keeps the
 *     group (`or $a0,$a0,$v1`), so the candidate needs the temporary
 *     (`term = ...; term |= ...; value |= term;`).
 *   - `((u64)1 << shift) << 14` is folded by gcc into `1 << (shift+14)`
 *     (`addiu $a0,$zero,0x4000`), so the shift has to stay 32-bit inside
 *     the cast.
 *   - moving the count load, reordering the declarations and using
 *     `long long` instead of `unsigned long long` change nothing.
 *   - the store order of the two byte fields is source-order sensitive:
 *     writing byteD before byteC is what gives the retail's byteC, byteD.
 *   - `-fno-schedule-insns`, `-fno-schedule-insns2`, `-fno-gcse`,
 *     `-mno-split-addresses` and `-fno-expensive-optimizations` all leave
 *     the constant in $a0: the allocation is decided before the scheduler.
 *
 * Unblocker: the exact source spelling that makes the reload of the 64-bit
 * copy of a0 land in $a0 (the argument's own register) while the constant 1
 * takes $t2.  The two terms must be built in the retail's order but the
 * reload order has to differ from this candidate's.
 */

typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned char u8;
typedef unsigned long long u64;

/* One GS texture upload slot: the queue is a table of 16-byte records. */
typedef struct {
    u32 word0;
    u16 half4;
    u16 half6;
    u32 word8;
    u8 byteC;
    u8 byteD;
    u16 halfE;
} GsTextureUploadSlot;

extern GsTextureUploadSlot D_001B9340[];

extern u32 D_001B15FC __attribute__((sda));

u64 func_00294D80(int a0, int a1, u32 a2, u32 a3, int arg5, int arg6)
{
    int shift;
    u64 value;
    u64 term;
    int count;

    shift = a0 - 6;
    shift = (shift >= 0) ? shift : 0;
    value = (u64)(1 << shift) << 14;
    value |= (u64)(arg6 >> 8);
    term = (u64)a0 << 26;
    term |= 0x01300000;
    value |= term;
    value |= (u64)a1 << 30;
    term = (u64)(arg5 >> 8) << 37;
    term |= 0x400000000ULL;
    value |= term;
    value |= 0x8000000000000000ULL;
    count = D_001B15FC;
    if (count < 0x40) {
        D_001B9340[count].word0 = a2;
        D_001B9340[count].half6 = arg5 >> 8;
        D_001B9340[count].half4 = 0;
        D_001B9340[count].word8 = a3;
        D_001B9340[count].halfE = arg6 >> 8;
        D_001B9340[count].byteD = a1;
        D_001B9340[count].byteC = a0;
        D_001B15FC = count + 1;
    }
    return value;
}
