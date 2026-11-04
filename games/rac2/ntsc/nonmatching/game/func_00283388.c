/* func_00283388 is 48 bytes at the boot and all 27 level placements. Its
 * body uses VU0 operations (vmulx, vmr32, vmove and vaddw) to form four
 * vectors, then stores them with sqc2. COP2/VU0 instructions are required;
 * the project compiler emits none from C. No C attempt is appropriate. Keep
 * INCLUDE_ASM pending maintainer agreement for ASM_FUNC, as recorded in
 * blocker 52.
 */
