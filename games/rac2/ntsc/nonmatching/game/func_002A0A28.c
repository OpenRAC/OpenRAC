/*
 * func_002A0A28: 352-byte USA v1.01 boot body, retained as INCLUDE_ASM.
 *
 * Retail uses $at as a live pointer from +0x0C through the table scan and
 * subsequent loops. It also uses multiple trapping `addi` operations,
 * including pointer updates at +0x0C and +0x20. Ordinary C in the
 * project compiler cannot request this register allocation or trapping
 * arithmetic without unsupported register pins or fake dependencies. This is
 * evidence for handwritten assembly, pending maintainer agreement for
 * ASM_FUNC; no C attempt was made and the source marker remains unchanged.
 */
