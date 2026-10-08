/* NON_MATCHING func_L15_002ED3A0 -- src/overlays/l15_quartu/vendor_0029C1D0.c
 * Best so far: SIZE ours 124 / retail 128, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   ## What it does
 *   Looks up an index in the D_L15_001AC140 array. If the value is non-zero, branches to func_L15_002ED3C4 with th
 *   ## Issue
 *   The retail code uses bnez (branch if not equal to zero) to jump directly to func_L15_002ED3C4 without a functi
 *   ## Wall hit
 *   GCC 2.95.3 does not support tail call optimization for this function pattern. The original code uses a conditi
 *   Joined retry p4, p6, p8 produce identical 124-byte code versus 128 retail.
 *   Missing ID preservation move; base load uses a separate lui register and is scheduled across a constant. Unblo
 */
extern int D_L15_001AC140[];
extern int D_L15_00160058_m __asm__("D_L15_00160058") NOT_SDA;
/* counts absent or inactive objects in a terminated id list */
int func_L15_002ED3A0(int index) {
 unsigned short *ids=(unsigned short *)D_L15_001AC140[index];
 int count=0; int id; char *base;
 if (!ids) return 0;
 base=(char *)D_L15_00160058_m;
 do { char *m; id=*ids; m=(id&0x7FFF)*256+base;
 if (!m || (unsigned char)m[0x20]==254 || (unsigned char)m[0x20]==253) ++count;
 ++ids; } while ((short)id>=0);
 return count;
}
