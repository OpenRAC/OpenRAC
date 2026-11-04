#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "asm_functions/executable.h"
#include "data/executable.h"

/*
 * core_text object 0x11CCE0-0x11D0D0. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Sony's EE kernel library (libkernl), eeloadfile.o: _lf_bind,
 * _lf_version, _sceSifLoadModuleBuffer and sceSifLoadModuleBuffer. The
 * linker dead-stripped the rest of the member; func_0011D098 is the seven
 * one-word remnants it left. Built with Sony's 2.9-ee (Makefile.sn,
 * EE29_CORE), like the prebuilt libkernl.a.
 */

extern int sceSifBindRpc();

extern int lf_bind_state;
extern char lf_rpc_client[];
extern char lf_rpc_buffer[];
extern char lf_iop_version[];

/* _lf_bind (libkernl.a eeloadfile.o): unless already bound (lf_bind_state
 * >= 0), bind the IOP loadfile RPC server (sceSifBindRpc, 0x80000006),
 * busy-waiting 0x100000 counts between tries until its server pointer
 * (+0x24) is set; a failed bind returns -1. Then mark it bound, call
 * function 0xFF (sif_call_rpc) for the 4-byte reply at lf_rpc_buffer
 * (-0x10001 if the call fails) and copy it to lf_iop_version.
 *
 * The success path has its own `return 0`: its $v0 = 0 is scheduled
 * ahead of the inlined memcpy, which moves the copy's registers up by
 * one as in retail. The already-bound test jumps to the shared one. */
int lf_bind(void) {
    int i;

    if (lf_bind_state >= 0)
        goto done;

    for (;;) {
        char *slot = lf_rpc_client;

        if (sceSifBindRpc(slot, 0x80000006, 0) < 0)
            return -1;

        if (*(int *)(slot + 0x24) != 0) {
            lf_bind_state = 0;
            if (sif_call_rpc(slot, 0xFF, 0, 0, 0, lf_rpc_buffer, 4, 0, 0) < 0)
                return 0xFFFEFFFF;
            memcpy(lf_iop_version, lf_rpc_buffer, 4);
            return 0;
        }

        for (i = 0x100000; i != -1; i--)
            ;
    }

done:
    return 0;
}

extern int lf_expected_version;

/* Twin of fs_version_mismatch on a different slot/handle pair. */
int lf_version_mismatch(void) {
    char *buf = sif_lib_version_string;
    char *slot = lf_iop_version;
    int r = 0;

    if (libc_memcmp(slot, buf, 4) != 0 && libc_memcmp(slot, lf_expected_version, 4) != 0) {
        r = libc_memcmp(buf, lf_expected_version, 4) != 0;
    }
    return r;
}

struct SifLoadCmd {
    int unk0;
    int unk4;
};
#define SIFCMD ((struct SifLoadCmd *)D_00158300)
/* _sceSifLoadModuleBuffer. Adapted from Lombyte's project (GPL-3.0) for PAL. */
int _sceSifLoadModuleBuffer(int arg0, int arg1, int arg2, void *out) {
    int *arg3 = out;
    int var_2_18;
    int temp;

    var_2_18 = 0xFFFF0000;
    if (lf_bind() < 0) {
        return var_2_18;
    }
    if (lf_version_mismatch() != 0) {
        return 0xFFFEFFFC;
    }
    SIFCMD->unk0 = arg0;
    if (arg2 != 0) {
        if (arg1 >= 0xFD) {
            memcpy(lf_rpc_buffer + 0x104, (void *)arg2, 0xFC);
            SIFCMD->unk4 = 0xFC;
        } else {
            memcpy(lf_rpc_buffer + 0x104, (void *)arg2, arg1);
            SIFCMD->unk4 = arg1;
        }
    } else {
        SIFCMD->unk4 = 0;
    }
    if (sif_call_rpc(lf_rpc_client, 6, 0, lf_rpc_buffer, 0x200, lf_rpc_buffer, 8, 0, 0) < 0) {
        return 0xFFFEFFFF;
    }
    temp = SIFCMD->unk0;
    *arg3 = SIFCMD->unk4;
    return temp;
}

extern int _sceSifLoadModuleBuffer(int arg0, int arg1, int arg2, void *arg3);

int sif_load_iop_module_buffer(int arg0, int arg1, int arg2) {
    char buf[0x10];
    return _sceSifLoadModuleBuffer(arg0, arg1, arg2, buf);
}

LINKER_REMNANT("asm/remnants/core_text", func_0011D098);
