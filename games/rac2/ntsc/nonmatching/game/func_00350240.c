/* Near miss: 304 bytes, expected 312, 0 of 28 places (USA v1.01 boot).
 *
 * Profile: -O2 -G8 -ffunction-sections (D_001B23CC and the transfer pointer
 * are small data).
 * Last verdict: `python scratch/wslcmp.py src/game/0034CEA0-g8.c func_00350240`
 *   func_00350240: 0 of 28 places match   [boot: 304 bytes, expected 312]
 *
 * Remaining difference: the third call's ring pointer.  Retail materialises
 * `lui a0,0xD; ori a0,0x9048` again for func_00351868 and adds it to a fresh
 * `lw v0,D_001B23CC($gp)`; this cc1 keeps 0xD9048 in a saved register and
 * reuses it (`addu a0,a0,s5`), which is two instructions shorter.  Both
 * transfer calls, the two engine addresses and the return comparison match.
 * A 28 Oct 2026 probe gave that call a second `SDATA` view of the same
 * measured pointer and used it only for the ring-pointer expression. It grew
 * to 300/312 bytes and remained 0/28, so the alias did not recover retail's
 * sequence in this unit. On 29 Oct 2026, a volatile array-element declaration
 * produced 308/312, 0/28; changing to a scalar volatile pointer object and
 * replacing `[0]` reads with the equivalent pointer read produced 296/312,
 * 0/28, but emitted absolute-address loads instead of the retail `$gp` load.
 */
typedef unsigned char u8;
typedef unsigned int u32;

extern char *D_001B23CC[] __attribute__((sda));
/* Reads one ring slice into the object: the two ranges the owner reports are
   turned into engine addresses and moved by the transfer helper, then the
   transfer is committed when the owner still holds the data. */
extern char D_001AE830[];

extern void func_00351848(unsigned char *, int *, int *, int *, int *);
extern int func_003518F0(char *, unsigned long long, unsigned long long, int, int);
extern void func_00351868(char *, int);
extern void func_0034F780(int);
extern int func_00350460(u32, int, u32, int, u32, int, int, int);

int func_00350240(u8 *unused, char *src, char *dst)
{
    int first_address, first_count, second_address, second_count;
    int used = *(int *)(src + 8);
    int remain = *(int *)(src + 0xC);
    int size;
    int result;

    size = (int)dst + *(int *)(dst + 0x50008);
    size -= used;
    if (remain < size)
        size = remain;
    remain -= size;
    func_00351848((unsigned char *)(D_001B23CC[0] + 0xD9048),
                  &first_address, &first_count, &second_address, &second_count);
    result = func_00350460((first_address & 0x0FFFFFFF) | 0x20000000, first_count,
                           (second_address & 0x0FFFFFFF) | 0x20000000, second_count,
                           used, size, (int)dst, remain);
    if (result > 0) {
        if (func_003518F0(D_001B23CC[0] + 0xD9048,
                          *(unsigned long long *)(src + 0x10),
                          *(unsigned long long *)(src + 0x18),
                          first_address, result) == 0)
            func_0034F780((int)D_001AE830);
        func_00351868(D_001B23CC[0] + 0xD9048, result);
    }
    return result > 0;
}
