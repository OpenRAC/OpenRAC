# First RAC2 C lot - 2026-10-01

Seven project-authored candidates compiled and linked with zero byte differences,
and were reconstructed again in a new private run. The original saved Ghidra GC
programme was inspected read-only; its hash matches the pinned retail boot.

| Symbol | Body bytes | Observed operation |
| --- | ---: | --- |
| FUN_00115200 | 12 | Return pointer from absolute cell |
| FUN_00115210 | 12 | Return address of an absolute pointer cell |
| FUN_00120BC8 | 8 | Return without effects |
| FUN_00125960 | 12 | Return an absolute data address |
| FUN_0012F9A8 | 12 | Read a pointer at argument offset 0x40, then its 32-bit word |
| FUN_0026F710 | 8 | Return zero |
| FUN_0026F718 | 8 | Return without effects |

Total: 72 bytes. Boundaries exclude alignment padding and include the return
delay slots. The linked candidate's STT_FUNC sizes agree with those boundaries;
the gate does not use the next arbitrary label as a function end.

The 64-bit zero-return trial failed: SN GCC produced `por` instead of `daddu`.
The 32-bit zero-return declaration matches. This is an instruction-generation
observation, not proof of the original source-level return type. The same
distinction applies to the opaque pointer and structure declarations.

Compiler profile is limited to these seven leaf functions. Tool fingerprints,
source fingerprint and each pair of complete body fingerprints are stored in
`progress/candidates.json`. A changed first getter (`pointer + 1`) was rejected
with exit code 1 because its complete function length differs. Synthetic tests
also reject stale, zero-sized, absolute, non-function and prefix-only cases.

Initial state was **matched_unintegrated**. The subsequent explicitly authorized
integration now links the seven genuine C sections into the complete boot.
Both loaded segments and all seven post-link bodies match. See
`INTEGRATION-FIRST-LOT.md` and `progress/integration.json`. No native gameplay
is verified; only those 72 C bytes are promoted to decomp.dev.
