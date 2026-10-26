#include "common.h"
#include "records.h"
#include "structs.h"

/*
 * cmecpu.cpp in the original source; text 0x1EE9F8-0x1EFD70.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/* Declarations in scope here before the split. */
extern char voice_pool[];
extern int bank_sound_count;
extern void FastMemCopy(void *, void *, int);
extern char backup_cam[];
extern char gCamBackupState[];
extern void *cur_cam NOT_SDA;
extern DispatchRec camera_types[];

ASM_FUNC("asm/handwritten/text", func_001EE9F8);
