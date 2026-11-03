#include "common.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/misc.h"
#include "asm_functions/executable.h"
#include "core/sif/load_module.h"

/*
 * init.cpp in the original source; text 0x201D58-0x201E10.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

extern int sce_sif_alloc_iop_heap(int arg0);

extern void sce_sif_free_iop_heap();

/* LoadIRXModule(name, args): starts an IOP module load through an RPC
   helper, hands sce_sif_set_dma a 4-word tagged buffer, polls the returned
   id until it is ready, then starts the module and reports the result.
   The nops before the poll loop's branch are the short-loop padding
   (tools/fix_short_loops.py); the result test comes before the
   sce_sif_free_iop_heap call, where retail's movz sits in its delay slot. */
int LoadIRXModule(void *arg0, void *arg1) {
    int buf[4];
    int result;
    int modid;
    int status;
    int ret = 1;

    result = sce_sif_alloc_iop_heap((int)arg1);
    buf[0] = (int)arg0;
    buf[1] = result;
    buf[2] = (int)arg1;
    buf[3] = 0;
    modid = sce_sif_set_dma(buf, 1);
    if (modid != 0) {
        while (sce_sif_dma_stat(modid) >= 0) {
        }
        status = sif_load_iop_module_buffer(result, 0, 0);
        if (status < 0) {
            ret = 0;
        }
        sce_sif_free_iop_heap(result);
    }
    return ret;
}
