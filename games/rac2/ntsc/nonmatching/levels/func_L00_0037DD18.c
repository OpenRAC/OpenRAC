/* GPT-6.1 Sol low tier, v0419, USA v1.01 pinned toolchain, default flags.
 * Best check: 100 bytes against 100, 0 of 27 places match.
 * An explicit first-slot case followed by a do/while search gives retail's
 * duplicated stores and top-of-loop bounds check. Still differs in base
 * materialization, register allocation, store scheduling, and strength reduction:
 * the loop walks a pointer and byte offset rather than scaling the index.
 * -fno-strength-reduce with this wording: 92 bytes, 0/27; the scaled index
 * returns but base placement and store order still differ. Page-layout spelling
 * gives 112 bytes; direct extern record/word arrays give 120 with that flag.
 * Next useful experiment needs an evidenced way to retain the scaled index
 * without changing the memory semantics. Declarations required are below.
 *
 * GPT-6.1 Sol high tier, v0900, 29 October 2026: the retained default
 * candidate still checks at 100/100 bytes, 0/27 places. Keep this smaller
 * source rather than replacing it with the new 104-byte variant.
 * New measured constraints on the pinned toolchain:
 * - Integer-address spelling of the retained byte-array view is still 100
 *   bytes and still strength-reduced; -fno-rerun-loop-opt is unchanged.
 * - A typed external Entry array, with a local first-entry pointer and
 *   D_L00_00231340[0].field00 as the first store, plus a local pointer for
 *   the final stores and -fno-strength-reduce, gives 104 bytes. It restores
 *   t2 as the high-address copy, t0 as the index and sll a3,t0,4. The loop
 *   hoists base+4 before the head, then rematerializes base after the search;
 *   retail rematerializes base inside the loop. Store scheduling also differs.
 * - Casting the typed table address to unsigned int before adding i<<4
 *   keeps the loop load at +4 rather than 0, but remains 104 bytes with a
 *   pre-loop base copy. No extra accesses or volatile declarations were used.
 * - On the typed-array variant, -frac2-gcse-unsplit-edges and
 *   -fno-expensive-optimizations leave the 104-byte output unchanged;
 *   -fno-gcse gives 108 and -fno-rerun-cse-after-loop gives 116 bytes.
 * - The incomplete-array sda declaration in a G0/no-strength-reduce unit
 *   still gives 92 bytes and introduces an unwanted assembler-at access.
 * Next useful experiment needs new evidence for the invariant base's live
 * range and rematerialization; ordinary loop/address spelling is exhausted.
 */

typedef unsigned char u8;
typedef unsigned int u32;
typedef int s32;

typedef struct {
    u32 field00, field04;
    s32 field08;
    u8 gap0c[4];
} Entry;

extern u8 D_L00_00231340[];

void func_L00_0037DD18(int a0, int a1, int a2)
{
    Entry *entry = (Entry *)D_L00_00231340;
    int i = 0;
    if (entry[0].field04 == 0) {
        entry[0].field00 = a1;
        entry[0].field08 = a2;
        entry[0].field04 = a0;
    } else {
        do {
            i++;
            if (i >= 32) return;
        } while (entry[i].field04 != 0);
        entry[i].field08 = a2;
        entry[i].field04 = a0;
        entry[i].field00 = a1;
    }
}
