/* Retained C, not matched. GPT-6.1 Sol worker, 28 October 2026.
   Profile: -O2 -G0 -ffunction-sections -fno-strength-reduce.
   USA v1.01: 184/192 bytes; 0/27 places match.
   Separate cleanup cases recover the likely mismatch branch and the loop-tail
   counter pattern. Retail retains an a0 copy of the record for early stores,
   a non-likely flags branch, and a longer 0xFD branch path; this C folds them.
   A u32 pointer view, -fno-gcse and -fno-thread-jumps each give the same 184 bytes.
   Continue-based nested cleanup gives the previous 180-byte result instead.
   Next useful input: evidence for the original record/pointer lifetime or type.
   Required declarations follow. */

typedef unsigned char u8;
typedef unsigned int u32;
typedef signed int s32;

typedef struct {
    u32 field00, field04;
    s32 field08;
    u8 gap0c[4];
} OrbitalPairTableView16;

extern u8 D_L00_00231340[];
extern int func_00282948(void *p);

void func_L00_0037E000(u32 a0)
{
    OrbitalPairTableView16 *entry = (OrbitalPairTableView16 *)D_L00_00231340;
    int i;
    for (i = 0; i < 32; i++, entry++) {
        OrbitalPairTableView16 *current;
        u8 *p;
        if (entry->field04 != a0) continue;
        p = (u8 *)entry->field00;
        current = entry;
        if ((*(unsigned short *)(p + 0x34) & 0x1000) == 0) {
            current->field04 = 0;
        } else if (p == 0 || p[0x20] == 0xFE) {
            current->field04 = 0;
        } else if (p[0x20] == 0xFD) {
            current->field04 = 0;
        } else if (func_00282948((char *)entry + 8) != 0) {
            entry->field04 = 0;
        }
    }
}
