#include "common.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/misc.h"
#include "core/kernel/kprintf.h"
#include "asm_functions/executable.h"

/*
 * stash.cpp in the original source; text 0x233FF8-0x234380.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

extern void sceSifInitRpc(int arg0);

void initialize_sif_rpc(void) {
    sceSifInitRpc(0);
}

extern int sceSifBindRpc(void *client, int id, int mode);

/*
 * Authentic structs from IOPSTASH.IRX STABS debug symbols:
 *   StashInfo: base, size, cd[10], free (cur), block (count)
 *   StashBlock: ram (unk_00), qwc (unk_04), comment (unk_08), pad (unk_0C)
 */
typedef StashInfo StashState_00234018;
extern StashState_00234018 stash_state_alias __asm__("D_001DD530");
typedef StashBlock Rec10_00234018;
extern Rec10_00234018 stash_entries_alias[] __asm__("D_001DD568");

/* init_stash: binds the IOP stash RPC server (0x11, no-wait mode; a bind
   error hangs), waits for the bind and retries after a delay loop until
   the server answers, then asks it (RPC 2, sceSifCallRpc's nine
   arguments) for the buffer's base and size and clears the 0x40 slots.
   The server check reads through a pointer set inside the loop: loop.c
   then hoists the struct's full address into a register of its own, as
   retail has it, while the client's address stays a constant. */
void init_stash(void) {
    StashState_00234018 *s;
    int reply[4];
    int i;

    for (;;) {
        if (sceSifBindRpc(stash_state_alias.cd, 0x11, 1) < 0) {
            for (;;) {
            }
        }
        while (sce_sif_check_stat_rpc(stash_state_alias.cd) != 0) {
        }
        s = &stash_state_alias;
        if (*(int *)((char *)s->cd + 0x24) != 0) {
            break;
        }
        i = 0xFFFF;
        while (i--) {
        }
    }
    sif_call_rpc(stash_state_alias.cd, 2, 0, 0, 0, reply, 0x10, 0, 0);
    stash_state_alias.base = reply[0];
    stash_state_alias.size = reply[1];
    stash_state_alias.free = reply[0];
    stash_state_alias.block = 0;
    for (i = 0; i < 0x40; i++) {
        stash_entries_alias[i].ram = 0;
        stash_entries_alias[i].qwc = 0;
    }
}
__asm__(".section .text\n\tnop\n");

extern StashBlock stash_entries[];

/* The stash: a buffer on the IOP side (base, size, from init_stash's
   RPC), the SIF RPC client at stash_rpc_client_address, and a bump allocator over the
   buffer that hands out up to 0x40 slots in stash_entries. */
extern StashInfo stash_state;

/* Stash_SendData: DMA arg1 quadwords from arg0 to the stash's next free
   address (sce_sif_set_dma takes a {src, dst, size, mode} record), reserve
   arg2 quadwords there, and return the slot. `h->block++` read into the
   slot index is what keeps retail's copy of the old count. */
int Stash_SendData(int arg0, int arg1, int arg2, int arg3) {
    StashInfo *h = &stash_state;
    int dma[4];
    int n;

    if (h->size - (h->free - h->base) < arg2 * 16) {
        return -1;
    }
    if (h->block == 0x40) {
        return -2;
    }
    dma[0] = arg0;
    dma[1] = h->free;
    dma[2] = arg1 * 16;
    dma[3] = 0;
    sce_sif_set_dma(dma, IOP_STASH_FETCH);
    n = h->block++;
    stash_entries[n].ram = h->free;
    stash_entries[n].qwc = arg2;
    stash_entries[n].comment = arg3;
    h->free += arg2 * 16;
    return n;
}

extern int stash_get_reserved_qwords(unsigned int);

extern char stash_rpc_client_address[];

/* stash_receive_data(dest, slot, offset, size, mode): size -1 means the
   slot's whole length (stash_get_reserved_qwords). -3 for a bad or empty slot, -1
   if offset + size runs past the slot. Otherwise the data is fetched
   from the stash (slot base + offset quadwords) to dest by the stash
   client's RPC 1 (IOP_STASH_FETCH), at most 0xFFFF quadwords per call; returns 0. The RPC
   is sceSifCallRpc, with nine arguments: end function and end
   parameter 0, the last on the stack. */
int stash_receive_data(void *dest, unsigned int slot, int offset, int size, int mode) {
    int src;
    int chunk;

    if (size == -1) {
        size = stash_get_reserved_qwords(slot);
    }
    if (slot >= 0x40) {
        return -3;
    }
    if (stash_entries[slot].qwc == 0) {
        return -3;
    }
    if (stash_entries[slot].qwc < offset + size) {
        return -1;
    }
    src = stash_entries[slot].ram + offset * 16;
    while (size != 0) {
        chunk = (size > 0xFFFF) ? 0xFFFF : size;
        sif_call_rpc(stash_rpc_client_address, IOP_STASH_FETCH, mode, &src, 0x10, dest, chunk * 16, 0, 0);
        size -= chunk;
        dest = (char *)dest + chunk * 16;
        src += chunk * 16;
    }
    return 0;
}

int stash_get_reserved_qwords(unsigned int arg0) {
    if (arg0 >= 0x40) {
        return -3;
    }
    return stash_entries[arg0].qwc;
}
