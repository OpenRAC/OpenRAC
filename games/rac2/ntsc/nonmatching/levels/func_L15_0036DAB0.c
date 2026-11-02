/* 0 of 2 places match - no C can be placed over the whole extent.
   The extent starts 0x10 early: its first four words are sw $t3,0x40($v1); nop;
   addiu $sp,$sp,0x20; nop - a tail plus padding whose registers ($t3, $v1) are never
   set here. The real body runs from 0x0036DAC0 (lui $a1,0x14 ...) and is a
   0x10-byte-shorter function; the same 16-byte lead-in was reported for other waves'
   catalogue splits. Recorded for the maintainers. */

/* body to place starts at +0x10 */

/* Fresh 23 October 2026 reconstruction appended below the historical note.
   Game -O2 -G8 with -frac2-qcopy -frac2-qcopy-barrier: 272-byte C body versus
   the catalogue's 288-byte extent, 0/2 placements in each of USA v1.01,
   USA v2.00 and PAL. Retail v1.01 starts with a 16-byte preceding tail:
   sw t3,0x40(v1); nop; addiu sp,sp,0x20; nop. The actual body starts at +0x10.
   One body trial reconstructs the menu records/search; relative body differences
   still include template load/store GPR order and the first GP load schedule.
   Reopen after maintainers verify/correct the catalogue entry and its references;
   the leading bytes cannot be produced by this ordinary C ABI body. */
typedef struct { short id; short type; void *label; short value; short pad; } MenuEntry;
typedef struct { short id; short pad; short value; char rest[6]; } MenuTemplate;
typedef struct { char pad[0x230]; void *current; } MenuOwner;
extern int D_00139948[] __attribute__((nosda));
extern MenuTemplate D_L15_002A0020[];
extern MenuEntry D_L15_0021D8F0[];
extern char D_L15_0029BAC0[];
extern void **D_L15_001AA7F0 __attribute__((sda));
extern MenuOwner D_L15_001C9DA0;

/* The retail catalogue includes a preceding 16-byte tail before this body. */
int func_L15_0036DAB0(char *object)
{
    int i = 0;
    while (i < 28 && D_00139948[i]) {
        int id = D_00139948[i];
        D_L15_0021D8F0[i].type = 3;
        D_L15_0021D8F0[i].label = D_L15_0029BAC0;
        D_L15_0021D8F0[i].value = D_L15_002A0020[id].value;
        D_L15_0021D8F0[i].id = D_L15_002A0020[id].id;
        ++i;
    }
    D_L15_0021D8F0[i].id = 0;
    *(int *)(object + 0x40) = 0;
    *(int *)(object + 0x30) |= 0x8000;
    i = 0;
    while (D_L15_001AA7F0[i]) {
        if (D_L15_001C9DA0.current == D_L15_001AA7F0[i]) {
            *(int *)(object + 0x40) = i;
            break;
        }
        ++i;
    }
    return 0;
}
