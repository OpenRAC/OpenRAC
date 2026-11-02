/* GPT-6.1 Sol (low), task v0438, 28 October 2026.
   Pinned game profile: -O2 -G8 -ffunction-sections; 44/40 bytes,
   0 of 27 USA v1.01 places match (tools/check.py --diff).
   Retail uses ADDA.S $f3,$f3 followed by MADD.S $f1,$f1,$f2;
   plain C emits MUL.S/ADD.S, different registers and return scheduling.
   The old draft incorrectly interpreted ADDA.S as multiplication:
   0x3E800000 is 0.25f, so ADDA.S adds 0.25f + 0.25f = 0.5f.
   This is a semantic approximation; accumulator arithmetic and conversion
   rounding must be preserved by any eventual handwritten implementation.
   Needs the sda extern below and a -G8 unit. No inline assembly attempted.
   Next useful step: maintainer review of handwritten classification;
   ordinary C has no FP-accumulator operation in this compiler profile. */
extern float D_001A7914 __attribute__((sda));

int func_L00_002EDD00(int value) {
    return (int)((float)value * D_001A7914 + 0.5f);
}
