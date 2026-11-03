#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "core/kernel/exit.h"

/*
 * crt0 (0x12D868-0x12DA38): the program entry point. The ELF's e_entry is
 * 0x12D868, and _exit ends right before boot.cpp. Name and extent from
 * the NTSC split, shifted by the entry point (+0x140).
 */

ASM_FUNC("asm/handwritten/core_text", func_0012D868); /* _start */

/* The CRT exit entry discards its argument and exits with status zero. */
void func_0012DA28(int status) {
    Exit(0);
}

ASM_FUNC("asm/handwritten/core_text", func_0012DA30);
