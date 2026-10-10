/* NON_MATCHING func_L00_0024BAE8 -- src/overlays/shared/menu_00249720.c
 * Best so far: SIZE ours 204 / retail 200, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Updates the selected menu record and chooses states 3, 12, 19, or 16 from menu fields.
 *   p2, p5 and p6 produce identical 200-byte code, with pointer and comparison-one registers a0/a1 swapped through
 *   Unblock requires an allocator idiom preserving pointer in a0 and one in a1 without hoisting the constant.
 */
extern int D_0013D390[];
extern int D_0015EFB0 MACRO_ADDR;
/* advances a menu record and selects its next state */
void func_L00_0024BAE8(void) {
 int one=1; char *p=(char *)D_0013D390;
 if (*(int *)(p+0xDC)>=3 || *(int *)(p+0xE4)>=0) { int index=*(int *)(p+0xCC); int *item; p+=0xB0; item=(int *)(index*192+p); if (*item==one) *item=2; }
 p=(char *)D_0013D390;
 if (*(int *)(p+0x1C)<-1 || *(int *)(p+0xEC)) { D_0015EFB0=3; return; }
 if (*(int *)(p+0x14)==-2) { if (*(int *)(p+0xC)+*(int *)(p+0xAC)<350) { D_0015EFB0=19; return; } D_0015EFB0=12; return; }
 if (*(int *)(p+0x14)>=-1) D_0015EFB0=16;
}
