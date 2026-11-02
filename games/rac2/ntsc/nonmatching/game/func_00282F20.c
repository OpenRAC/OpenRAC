/* func_00282F20 - 80 bytes, 28 placements (boot and all 27 levels).
   A current-master stub check reports 0/28 matches. The boot and all USA
   v1.01 level bodies are identical. The body loads a vector with `lqc2`,
   performs VU0 vector operations and reciprocal square root (`vrsqrt`,
   `vwaitq`), transfers through `qmtc2.ni`/`qmfc2.ni`, then multiplies and
   stores with `sqc2` in the live `jr $ra` delay slot. Plain C cannot emit
   these instructions. This is handwritten-assembly evidence, not a C match;
   retain the source marker pending maintainer agreement before using
   `ASM_FUNC`.

   The extent includes an additional zero-vector store-and-return sequence at
   +0x40..+0x4C. Keep the catalogue and marker unchanged pending review of the
   measured full extent.
*/
