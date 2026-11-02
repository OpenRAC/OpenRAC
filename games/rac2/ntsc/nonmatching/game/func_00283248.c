/* func_00283248 - 280 bytes, boot + 27 levels, USA v1.01.
 *
 * Retail inspection on 29 October 2026 shows `qmtc2.ni` at +0x78 and a
 * sequence of COP2/VU0 vector operations through +0xFC, including `vaddx`,
 * `vmul`, `vmadd*` and `qmfc2.ni`. This is evidence for hand-written assembly;
 * ordinary C cannot emit this instruction sequence. Keep the INCLUDE_ASM
 * marker until a maintainer agrees to classify it with ASM_FUNC.
 */
