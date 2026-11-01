/* func_00350190 - 76 bytes, 28 places (the boot and every level).

   28 October 2026 current-master check in its original translation unit:
   68-byte candidate, 0/28 USA v1.01 placements. Rechecked by temporarily
   restoring this definition at its marker: the boot and all 27 levels report
   68 bytes, expected 76. The diff shows differences in both global-address
   sequences, the helper-call relocation, and the ending.
   The retail body ends with the privileged instructions:

       0x003501c4  0000000f  sync
       0x003501c8  42000038  ei

   The current check does not reproduce the global-address setup or helper-call
   relocation, so the earlier note that only the ending differed is stale.

   `sync` and `ei` are privileged R5900 instructions that this compiler never
   emits.  Measured: `__builtin___sync_synchronize()` compiles to a call to
   an undefined `__builtin___sync_synchronize` symbol. The standard
   `__sync_synchronize()` spelling likewise becomes a call to `func_000000`
   followed by a nop in the original unit; it does not emit `sync`. `strings`
   on build/compiler/tools/cc1 shows no `sync` builtin. The only C spelling of
   these instructions is inline `asm`, which AGENTS.md forbids in src/.

   What would unblock it: a maintainer decision to accept this body as
   handwritten assembly (ASM_FUNC), because it uses instructions the
   compiler never emits; or a compiler option that lowers a barrier to
   `sync`/`ei`.

   Tried: the C below with the `sda` attribute on both globals (which is what
   gives the retail's `lui`/`lw` pairs and the store through `$at`).
*/

extern volatile int D_001AE7D8 __attribute__((sda));
extern char *D_001B23CC[] __attribute__((sda));
extern void func_00351FA0(unsigned char *);

int func_00350190(void)
{
    if (D_001AE7D8 != 0) {
        func_00351FA0(D_001B23CC[0] + 0xD9168);
        D_001AE7D8 = 0;
    }
    return 0;
}
