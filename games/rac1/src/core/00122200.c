#include "common.h"
#include "callees.h"
#include "structs.h"

/*
 * core_text object 0x122200-0x1224B0. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/DECOMP_PROGRESS.md.
 *
 * Sony's libgraph: graph005.o (sceGszbufaddr, func_00122200) and
 * graph006.o (sceGsSetDefDrawEnv, func_001222C8), back to back. Built with
 * Sony's 2.9-ee (Makefile.sn, EE29_CORE).
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
extern char lf_iop_version[];
extern int lf_expected_version;
extern int _sceSifLoadModuleBuffer(int arg0, int arg1, int arg2, void *arg3);
extern void tty_reset_init_flag(void);
extern void init_tlb_32mb(void);
extern void kernel_init_tlb(void);
extern int FirstSemaphore;
extern int SecondSemaphore;
extern int osd_syscall_patch_table[];
extern char osd_kernel_patch_code[];
extern int PatchIsNeeded(void);
extern void flush_cache(int);
extern int sce_cd_sync_s(int);
extern void *cd_user_callback;
extern int cd_callback_sema;
extern int cd_ncmd_sema;
extern int cd_scmd_sema;
extern int cd_ncmd_busy;
extern int cd_poweroff_installing;
extern int cd_poweroff_installed;
extern int sif_call_rpc();
extern int cd_ncmd_prechk(int);
extern void signal_semaphore(int);
extern char cd_ncmd_rpc_client[];
extern int cd_ncmd_recv_buffer;
extern void scePrintf();
extern int sce_sif_check_stat_rpc(void *);
extern char cd_scmd_wait_msg[];
extern char cd_scmd_rpc_client[];
extern int cd_debug_level;
extern int cd_scmd_prechk(int);
extern int cd_scmd_recv_buffer;
extern char CoreDataTable[];

extern void *sceGsGetGParam(void);

/*
 * sceGszbufaddr-style size: fbw = (w + 63) / 64 pages across; fbh =
 * (h + 63) / 64 for a psm with bit 1 set, else (h + 31) / 32; returned as
 * a short, doubled unless the first 8 bytes of sceGsGetGParam()
 * (func_00121D08) masked with 0x0000FFFF0000FFFF equal 1 (interlace mode
 * 1, field mode 0).
 *
 * The old decode's "clamps" are plain signed divisions by 64 and 32:
 * gcc's own expansion of `x / 64` is the `addiu; slt -1; movn` sequence
 * retail has, where the hand-written clamps always gave slti/movz (192
 * against 200). The GParam test has to be the explicit 64-bit mask (two
 * short field compares fold shorter, SIZE 196). Exact under 2.9-ee with
 * the `== 1` arm first; 2.95.3's best is 8/200, a prologue-order residual.
 */
short sceGszbufaddr(short psm, short w, short h) {
    unsigned long *g = sceGsGetGParam();
    int fbw, fbh;

    fbw = (w + 63) / 64;
    if (psm & 2) {
        fbh = (h + 63) / 64;
    } else {
        fbh = (h + 31) / 32;
    }
    if ((*g & 0x0000FFFF0000FFFFUL) == 1) {
        return fbw * fbh;
    }
    return fbw * fbh * 2;
}

/* Sony's sceGsDrawEnv1 (libgraph.h): eight (value, GS-register-address)
 * pairs, all 64-bit ("long" on this ABI). Offsets confirmed against the
 * SET_FRAME/SET_ZBUF/SET_XYOFFSET/SET_SCISSOR/SET_TEST bit layouts and
 * the FRAME_1/ZBUF_1/XYOFFSET_1/SCISSOR_1/PRMODECONT/COLCLAMP/DTHE/
 * TEST_1 register ids in eestruct.h. */
typedef struct {
    unsigned long frame1, frame1addr;
    unsigned long zbuf1, zbuf1addr;
    unsigned long xyoffset1, xyoffset1addr;
    unsigned long scissor1, scissor1addr;
    unsigned long prmodecont, prmodecontaddr;
    unsigned long colclamp, colclampaddr;
    unsigned long dthe, dtheaddr;
    unsigned long test1, test1addr;
} DrawEnv1_1222C8;

extern short sceGszbufaddr(short psm, short w, short h); /* sceGszbufaddr */

/* sceGsSetDefDrawEnv (libgraph.h): fills in a default drawing
 * environment for a psm/w/h framebuffer. zbuf1's page count reuses
 * func_00122200 (the same page-count helper FRAME's own fbw comes from)
 * on the color psm/w/h, since the Z buffer is laid out right after the
 * color buffer; ztest==0 disables Z (mask writes, ZTE/ZTST forced on).
 * prmodecont/colclamp are read-modify-write: only their AC/CLAMP bit is
 * set, whatever else the caller's memory already held is left alone. */
int sce_gs_set_def_draw_env(DrawEnv1_1222C8 *draw, short psm, short w, short h,
                   short ztest, short zpsm) {
    short fbw = ((w + 63) >> 6) & 0x3F;
    short zbp;

    draw->frame1addr = 0x4C;
    draw->frame1 = ((unsigned long)fbw << 16) | ((unsigned long)(psm & 0xF) << 24);
    draw->zbuf1addr = 0x4E;
    if (ztest == 0) {
        zbp = sceGszbufaddr(psm, w, h);
        draw->zbuf1 = (unsigned long)zbp | ((unsigned long)(zpsm & 0xF) << 24) | (1UL << 32);
    } else {
        zbp = sceGszbufaddr(psm, w, h);
        draw->zbuf1 = (unsigned long)zbp | ((unsigned long)(zpsm & 0xF) << 24);
    }
    draw->xyoffset1addr = 0x18;
    {
        long ofx = 2048L - (w >> 1);
        long ofy = 2048L - (h >> 1);
        draw->xyoffset1 = (unsigned long)(ofx << 4) |
                          ((unsigned long)(ofy << 4) << 32);
    }
    draw->scissor1addr = 0x40;
    draw->scissor1 = ((unsigned long)(w - 1) << 16) | ((unsigned long)(h - 1) << 48);
    draw->prmodecontaddr = 0x1A;
    draw->prmodecont |= 1;
    draw->colclampaddr = 0x46;
    draw->colclamp |= 1;
    draw->dtheaddr = 0x45;
    if (psm & 2) {
        draw->dthe |= 1;
    } else {
        draw->dthe &= ~1UL;
    }
    draw->test1addr = 0x47;
    if (ztest != 0) {
        draw->test1 = ((unsigned long)(ztest & 3) << 17) | 0x10000;
    } else {
        draw->test1 = 0x30000;
    }
    __asm__ __volatile__("sync");
    return 8;
}

INCLUDE_ASM("asm/nonmatchings/core_text", func_001224AC);
