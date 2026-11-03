#ifndef MACROS_H
#define MACROS_H

/*
 * Keeps a variable OUT of the small-data area.
 *
 * Retail addresses some globals via $gp (the MIPS small-data area, base
 * 0x00166D00 from its own .reginfo, window 0x15ED00..0x16ED00). To
 * reproduce those we must build with a nonzero -G, but that makes the
 * compiler use $gp for EVERY small global -- including the ~60 that live
 * far outside the window, whose references then can't reach
 * ("relocation truncated to fit: R_MIPS_GPREL16").
 *
 * Placement is decided per variable by its declared size, but the
 * incomplete-array trick (`extern char x[];`) changes how the variable
 * must be spelled at every use site, which is unacceptable for scalars
 * inside already-matching functions. An explicit section attribute
 * achieves the same thing while leaving use sites untouched: the
 * compiler knows the variable isn't in .sdata and falls back to lui/lo.
 *
 * Verified directly: a plain `extern unsigned char x;` compiles to
 * `lbu $v0,0($gp)` at -G8, the same declaration with this macro compiles
 * to `lui`/`lbu`. (`aligned` does NOT work -- it stays gp-relative.)
 */
#define NOT_SDA __attribute__((section(".data")))

/*
 * Loads a global with retail's one-register form:
 *     lui $2,%hi(D) / lw $2,%lo(D)($2)
 * where plain declarations give the split form this compiler prefers:
 *     lui $2,%hi(D) / lw $3,%lo(D)($2)
 *
 * The section name makes the compiler treat the symbol as small data, so
 * it emits the unsplit assembler macro `lw $2,D`. The declared size is
 * still over -G2, so the assembler does not use $gp for it and expands the
 * macro through the destination register. The register choice around the
 * load follows, because the compiler allocated one pseudo, not two.
 *
 * Loads only. A store to such a symbol becomes a two-instruction macro
 * (`lui $at` / `sw`) that the compiler still counts as one instruction, so
 * it can land in a delay slot; the assembler then warns "macro used after
 * .set nomacro". Check the make log for that warning.
 */
#define MACRO_ADDR __attribute__((section(".sdata")))

/*
 * A scalar in the file's own small data, declared with its real type:
 *
 *     extern float jump_speed SDATA(jump_speed);
 *
 * Retail reaches a level file's own tuning floats and ints through $gp at
 * every access, in one instruction. The older way to get that here is
 * `extern short D_x;` read as `*(float *)&D_x`, and for most functions it
 * matches. But a read through a cast is not a scalar to the compiler: it may
 * alias any store through a pointer, so the load is ordered behind such
 * stores and its value is forgotten at each one. Where retail loads the
 * global before a store through a struct pointer, only the real type
 * reproduces it (func_L17_002EEB08, func_L17_002EDE50), with the stores
 * written as struct members.
 *
 * A readable name expands to its address first (names.h), so the label is the address's.
 * The declaration gives the symbol the assembler label `sym__gp`;
 * tools/check_macro_slots.py makes every such label a 1-byte .extern equated
 * to the symbol, so the assembler emits the $gp-relative form. A file that
 * also declares the same symbol `extern short` for an older function needs
 * two C names for it; keep the real name for this declaration.
 */
#define SDATA(sym) SDATA_LABEL(sym)
#define SDATA_LABEL(sym) __asm__(#sym "__gp") MACRO_ADDR

/* Puts a block of data in a section of its own, so the linker can place it in retail's hole (tools/data_in_c.py). */
#define DATA_SECTION(label) __attribute__((section(".data." #label)))

/* The number of elements in an array. */
#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

/*
 * A field at a raw offset, for when the struct is not known yet. Use a real field when you can.
 * STRUCT_OFFSET is easy to search for, so these can be replaced later.
 */
#define STRUCT_OFFSET(pointer, offset, type) (*(type *)((char *)(pointer) + (offset)))

#endif /* MACROS_H */
