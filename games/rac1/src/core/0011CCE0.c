#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x11CCE0-0x11D0D0. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's EE kernel library (libkernl), eeloadfile.o: _lf_bind,
 * _lf_version, _sceSifLoadModuleBuffer and sceSifLoadModuleBuffer. The
 * linker dead-stripped the rest of the member; func_0011D098 is the seven
 * one-word remnants it left. Built with Sony's 2.9-ee (Makefile.sn,
 * EE29_CORE), like the prebuilt libkernl.a.
 */

/* Declarations in scope here before the split. */
extern long strtol(int arg0, int arg1, int arg2);
extern int SbrkErrno;
extern void *_impure_ptr NOT_SDA;
extern int __sread(void *arg0, void *arg1, void *arg2);
extern int __swrite(void *arg0, void *arg1, void *arg2);
extern long __sseek(void *arg0, void *arg1, void *arg2);
extern void __sclose(void *arg0);
extern void libc_fflush(void);
extern void _fwalk(void *, void *);
extern char locale_lconv[];
extern int libc_glue_read();
extern long func_00116108_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_00116108");
extern long func_001188C8_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_001188C8");
extern long func_00114518_wide(int *errOut, void *a, void *b, void *c)
    __asm__("func_00114518");
extern int _close_r(int *errOut, int arg1);
extern int __sfvwrite(int, void *);
extern void __sinit(void *);
extern void _vfprintf_r(void *, void *, int, int);
extern int libc_glue_write();
extern int CoreGlobalWord NOT_SDA;
extern void Deci2Call(int arg0, void *arg1);
extern void *InitBlockData NOT_SDA;
extern int sif_sregs[];
extern void disable_dmac_atomic(int);
extern void remove_dmac_handler(int, int);
extern int sif_cmd_dmac_handler_id;
extern int sif_cmd_init_done;
extern int sif_cmd_data_sys_handlers NOT_SDA;
extern int sif_cmd_data_usr_handlers NOT_SDA;
extern void sif_send_cmd_common(int, int, int, int, int, int, int);
extern void sceSifExitCmd(void);
extern int RpcCommandState NOT_SDA;
extern int disable_interrupts(void);
extern void enable_interrupts(void);
extern int create_semaphore(void *);
extern int fs_iob_sema;
extern int fs_iob_sema_2;
extern char fs_iob_table[];
extern int fs_call_sema;
extern void fs_call_sema_init(void);
extern int fs_call_sema NOT_SDA;
extern char sif_lib_version_string[];
extern char FsResetBuffer[];
extern int fs_expected_version;
extern void libc_memset(void *, int, int);
extern int FsResetState;
extern int iop_heap_bind_state;
extern char iop_heap_rpc_client[];
extern int iop_heap_rpc_reply;
extern int iop_heap_rpc_arg;
extern int fs_version_mismatch(void);

extern int sceSifBindRpc();
extern int sif_call_rpc();
extern void *memcpy(void *, const void *, unsigned int);

extern int lf_bind_state;
extern char lf_rpc_client[];
extern char lf_rpc_buffer[];
extern char lf_iop_version[];

/* _lf_bind (libkernl.a eeloadfile.o): unless already bound (D_0012FDB0
 * >= 0), bind the IOP loadfile RPC server (func_0011B2F8, 0x80000006),
 * busy-waiting 0x100000 counts between tries until its server pointer
 * (+0x24) is set; a failed bind returns -1. Then mark it bound, call
 * function 0xFF (func_0011B4C8) for the 4-byte reply at D_00158300
 * (-0x10001 if the call fails) and copy it to D_00158528.
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

/* Twin of func_0011BEB8 on a different slot/handle pair. */
int lf_version_mismatch(void) {
    char *buf = sif_lib_version_string;
    char *slot = lf_iop_version;
    int r = 0;

    if (libc_memcmp(slot, buf, 4) != 0 &&
        libc_memcmp(slot, lf_expected_version, 4) != 0) {
        r = libc_memcmp(buf, lf_expected_version, 4) != 0;
    }
    return r;
}

struct SifLoadCmd {
    int unk0;
    int unk4;
};
#define SIFCMD ((struct SifLoadCmd *) D_00158300)
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
            memcpy(lf_rpc_buffer + 0x104, (void *) arg2, 0xFC);
            SIFCMD->unk4 = 0xFC;
        } else {
            memcpy(lf_rpc_buffer + 0x104, (void *) arg2, arg1);
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
