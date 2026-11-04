/*
 * func_0029D398: 312-byte USA v1.01 boot body, left as INCLUDE_ASM.
 *
 * Retail transforms vertices using VU0/COP2 instructions (`lqc2`,
 * `qmtc2.ni`, `vitof0`, `vmulax`/`vmadd*`, `vftoi0`, `qmfc2.ni`) and MMI
 * color packing (`pextlb`, `pmulth`, `pmfhl.sh`, `psrah`, `ppacb`, `pcpyld`,
 * `ppach`). The C compiler cannot emit its VU0 instructions. This is evidence
 * for handwritten assembly, not a C candidate; retain the marker pending
 * maintainer agreement for ASM_FUNC.
 */
