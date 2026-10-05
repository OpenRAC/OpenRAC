/* NON_MATCHING func_L00_00267CC8 -- src/overlays/shared/stream_002670E0.c
 * Best so far: SIZE ours 68 / retail 64, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Fragment: ends in `jr $31` with no delay-slot instruction (the slot belongs to the next symbol), so the functi
 *   Body: ring-buffer index (head - min(a, tail) + 30) % 30 into a table at D_0013A5E0+0x2460, returning the slot 
 */
typedef struct { char pad[0x18E]; s16 head; s32 count; char pad2[0x2D0-0x194]; f32 v[30]; } Hist_266e80;
extern Hist_266e80 D_0013CA40;

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/overlays/shared/ui_menus_00266d60.c, FUN_L00_00266e80. */
f32 func_L00_00267CC8(s32 n) { s32 m; if (D_0013CA40.count < n) n = D_0013CA40.count; return D_0013CA40.v[(D_0013CA40.head - n + 30) % 30]; }
