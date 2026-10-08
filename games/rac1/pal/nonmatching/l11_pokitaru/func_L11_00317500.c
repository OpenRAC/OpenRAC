/* NON_MATCHING func_L11_00317500 -- src/overlays/l11_pokitaru/vendor_00312BD8.c
 * Best so far: BYTES 4/148 (97.3% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Counts class 1246 objects with states 13 through 17 using a signed-short count.
 *   p3, p5 and p6 produce identical 144-byte code versus 148 retail: ID preservation move is coalesced, changing c
 *   p4 is 4/148 bytes off, with andi a2,v0,0xFFFF instead of daddu; unblock with a plain C copy-conversion idiom.
 */
extern short *D_L11_001AC540[];
extern int D_L11_00160058_m __asm__("D_L11_00160058") MACRO_ADDR;
/* counts class 1246 objects in states thirteen through seventeen */
int func_L11_00317500(unsigned char *moby) {
 int count; unsigned short *ids; unsigned short id; char *base;
 if (moby[0x21]==255) return 0;
 ids=(unsigned short *)D_L11_001AC540[moby[0x21]];
 count=0; if (!ids) return 0;
 base=(char *)D_L11_00160058_m;
 for (;;) { char *m; unsigned int state; unsigned int raw=*ids; id=raw; m=base+(raw&0x7FFF)*256;
 if (*(short *)(m+0xA6)==1246) { state=(unsigned char)m[0x20]; if (state>=13) { if (state<18) count=(short)(count+1); } }
 if ((short)id<0) return count; ++ids; }
}
