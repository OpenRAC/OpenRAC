#include <stdarg.h>
#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "asm_functions/executable.h"
#include "data/executable.h"
#include "core/kernel/threads_deci2.h"

/*
 * core_text object 0x119D88-0x11CCE0. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Sony's EE kernel library (libkernl), several of its modules back to
 * back: the kernel printf (scePrintf, scePrintf, over the print core
 * kprintf_format), sifcmd (the command handler table and sceSifSendCmd),
 * sifrpc (the 0x40-byte packet table, sceSifBindRpc, sceSifCallRpc =
 * sif_call_rpc), the IOP file stubs (sceOpen, sceClose, sceRead, ... over
 * a 32-entry fd table) and the IOP heap stubs. Built with Sony's 2.9-ee
 * like the rest of the SDK (Makefile.sn, EE29_CORE): the sd/ld spills and
 * the frames of scePrintf and fs_new_iob are that compiler's.
 */

/* kputchar (libkernl.a:kprintf.o): wait until the SIO status port
   (0x1000F130) has its busy bit (0x8000) clear, write the character to
   the SIO data port (0x1000F180) and return it.

   Retail's store is not volatile (it fills the return's delay slot, which
   a volatile access never does) yet its address stays in a register
   (lui/ori). A plain constant address is folded into the store by the
   optimizer instead, so the address goes through an empty asm that hides
   its value. */
int kputchar(int c) {
    unsigned char *p;

    while (*(volatile unsigned int *)0x1000F130 & 0x8000)
        ;
    __asm__("" : "=r"(p) : "0"(0x1000F180));
    *p = c;
    return c;
}

extern int deci2_buffered_byte_count;
extern char deci2_output_buffer[];

/* deci2Putchar (libkernl.a:kprintf.o): the kprintf hook used for the
   deci2 debug console. Buffers characters into deci2_output_buffer (a 0x80-byte
   line buffer) counted by deci2_buffered_byte_count, flushing (kputs) and
   resetting the count once it reaches 0x7E; on '\n' it also flushes
   immediately (appending the '\n' first) instead of just buffering it. */
void deci2Putchar(int c) {
    int count = deci2_buffered_byte_count;

    if (count >= 0x7E) {
        deci2_buffered_byte_count = 0;
        deci2_output_buffer[0x7F] = 0;
        kputs(deci2_output_buffer);
        count = deci2_buffered_byte_count;
    }
    if (c == 0xA) {
        deci2_buffered_byte_count = 0;
        deci2_output_buffer[count] = c;
        deci2_output_buffer[count + 1] = 0;
        kputs(deci2_output_buffer);
    } else {
        deci2_buffered_byte_count = count + 1;
        deci2_output_buffer[count] = c;
    }
}

extern int kputchar(int c);

/* serialPutchar (libkernl.a:kprintf.o): the printf hook used when going to
   the serial port. '\n' is expanded to a CR/LF pair (write '\r' then the
   original character); anything else is written as-is via kputchar. */
void serialPutchar(int c) {
    if (c == 0xA) {
        kputchar(0xD);
        kputchar(0xA);
    } else {
        kputchar(c);
    }
}

/* Float printer: extract the exponent and hidden mantissa bit, retain two
   guard bits when shifting right, and round up when both are set.
   Uses the target compiler's 64-bit long and R5900 shift semantics. */
int kprintf_ftoi(unsigned long bits) {
    long exponent = (long)((bits << 1) >> 53);
    exponent -= 1075;
    if (exponent < -53)
        return 0;
    if (exponent >= 13)
        return 9999;
    bits = (bits << 12) >> 12;
    bits |= 1UL << 52;
    if (exponent < 0) {
        exponent = -exponent;
        bits >>= (int)(exponent - 2);
        if ((bits & 3) == 3)
            bits = (bits >> 2) + 1;
        else
            bits >>= 2;
    } else {
        bits <<= (int)exponent;
    }
    return (int)bits;
}

extern void *kprintf_putchar_hook;
extern long func_0011E6D8(double);
extern void kprintf(const char *, ...);

/* printfloat (libkernl.a:kprintf.o): the kprintf %f formatter. Prints a
   leading '-' through the putchar hook (kprintf_putchar_hook) if negative, then
   normalises the magnitude into [0.1, 1.0) by repeated *10 (if it
   started below 0.1) or /10 (if at or above 1.0), counting the power of
   ten removed in exp10, scales the mantissa to an integer (*1000000,
   func_0011E6D8, then ftoi = kprintf_ftoi) and prints it as "0.%d",
   followed by the exponent as "e+%d" or "e%d" (%d supplies the '-' for
   a negative exponent itself).

   The magnitude is checked, then rescaled, in nested `if (v < 0.1)
   { while (v < 0.1) ... }`: retail keeps a separate 0.1 for each. Its
   three format strings and its constants open this object's .rodata
   (significand_format-printfloat_mantissa_scale), followed by _printf's jump table
   (jtbl_001528B0): the strings are literals so that the table lands
   16-aligned at its retail address (config/core_rodata.txt). */
void printfloat(double v) {
    int exp10 = 0;
    int digits;

    if (v < 0.0) {
        v = -v;
        ((void (*)(int))kprintf_putchar_hook)(0x2D);
    }
    if (v < 0.1) {
        while (v < 0.1) {
            v = v * 10.0;
            exp10--;
        }
    } else if (v >= 1.0) {
        while (v >= 1.0) {
            v = v / 10.0;
            exp10++;
        }
    }
    digits = kprintf_ftoi(func_0011E6D8(v * 1000000.0));
    kprintf("0.%d", digits);
    if (exp10 >= 0) {
        kprintf("e+%d", exp10);
    } else {
        kprintf("e%d", exp10);
    }
}

/* _printf: the SDK libc formatter behind printf/sprintf: walks fmt, takes
   each argument from the 8-byte slots at ap and writes characters through
   the output hook at kprintf_putchar_hook, with interrupts held off around it.
   Adapted from Lombyte's project (GPL-3.0): src/sdk/debug/printf_impl.c, _printf. */

extern s32 disable_interrupts(void);
extern s32 EnableInterrupts_A0A0(void) __asm__("func_0011D9A8");
extern void printfloat_A0A0(double) __asm__("func_00119F38");
extern void (*kprintf_putchar_hook_r[])(s32) __asm__("D_0012FD00");

int kprintf_format(const char *fmt, va_list ap) {
    char buf[32];
    char *pad;
    char *p;
    s32 mod;
    s32 n;
    unsigned long d;
    u8 ch;
    char second;
    s64 v;
    s32 ie;

    ie = disable_interrupts();
    for (; *fmt != 0; fmt++) {
        p = (char *)fmt;
        pad = 0;
        mod = 0;
        if (*fmt == '%') {
        again:
            fmt++;
            switch ((char)(*fmt - '0')) {
                case 0:
                    n = fmt[1] - '0';
                    second = fmt[2];
                    if ((u8)n < 10) {
                        if ((u32)(second - '0') < 10) {
                            n = n * 10 + (second - '0');
                            fmt += 2;
                            if (n >= 32) {
                                n = 31;
                            }
                        } else {
                            fmt += 1;
                        }
                        pad = &buf[31 - n];
                        while (n > 0) {
                            buf[31 - n] = '0';
                            n--;
                        }
                        goto again;
                    }
                    goto again;
                case 'l' - '0':
                    mod = 'l';
                    goto again;
                case 'h' - '0':
                    mod = 'h';
                    goto again;
                case 'o' - '0':
                    if (mod == 'l') {
                        v = (ap += 8, *(s64 *)(ap - 8));
                    } else if (mod == 'h') {
                        v = (ap += 8, *(u16 *)(ap - 8));
                    } else {
                        v = (ap += 8, *(u32 *)(ap - 8));
                    }
                    p = &buf[31];
                    *p = 0;
                    if (v == 0) {
                        *--p = '0';
                    } else {
                        do {
                            *--p = (v & 7) + '0';
                            v = (u64)v >> 3;
                        } while (v != 0);
                    }
                    if (pad != 0 && pad < p) {
                        p = pad;
                    }
                    while (*p != 0) {
                        kprintf_putchar_hook_r[0](*p++);
                    }
                    break;
                case 'x' - '0':
                    if (mod == 'l') {
                        v = (ap += 8, *(s64 *)(ap - 8));
                    } else if (mod == 'h') {
                        v = (ap += 8, *(u16 *)(ap - 8));
                    } else {
                        v = (ap += 8, *(u32 *)(ap - 8));
                    }
                    p = &buf[31];
                    *p = 0;
                    if (v == 0) {
                        *--p = '0';
                    } else {
                        do {
                            d = v & 0xF;
                            if (d < 10) {
                                *--p = d + '0';
                            } else {
                                *--p = d + 'a' - 10;
                            }
                            v = (u64)v >> 4;
                        } while (v != 0);
                    }
                    if (pad != 0 && pad < p) {
                        p = pad;
                    }
                    while (*p != 0) {
                        kprintf_putchar_hook_r[0](*p++);
                    }
                    break;
                case 'd' - '0':
                    if (mod == 'l') {
                        v = (ap += 8, *(s64 *)(ap - 8));
                    } else if (mod == 'h') {
                        v = (ap += 8, *(s16 *)(ap - 8));
                    } else {
                        v = (ap += 8, *(s32 *)(ap - 8));
                    }
                    p = &buf[31];
                    *p = 0;
                    if (v == 0) {
                        *--p = '0';
                    } else {
                        if (v < 0) {
                            v = -v;
                            kprintf_putchar_hook_r[0]('-');
                        }
                        while (v != 0) {
                            *--p = v % 10 + '0';
                            v = v / 10;
                        }
                    }
                    if (pad != 0 && pad < p) {
                        p = pad;
                    }
                    while (*p != 0) {
                        kprintf_putchar_hook_r[0](*p++);
                    }
                    break;
                case 'u' - '0':
                    if (mod == 'l') {
                        v = (ap += 8, *(s64 *)(ap - 8));
                    } else if (mod == 'h') {
                        v = (ap += 8, *(u16 *)(ap - 8));
                    } else {
                        v = (ap += 8, *(u32 *)(ap - 8));
                    }
                    p = &buf[31];
                    *p = 0;
                    if (v == 0) {
                        *--p = '0';
                    } else {
                        do {
                            *--p = (u64)v % 10 + '0';
                            v = (u64)v / 10;
                        } while (v != 0);
                    }
                    if (pad != 0 && pad < p) {
                        p = pad;
                    }
                    while (*p != 0) {
                        kprintf_putchar_hook_r[0](*p++);
                    }
                    break;
                case 'e' - '0':
                case 'f' - '0': {
                    f32 f = (ap += 8, *(f32 *)(ap - 8));
                    if (f == 0.0f) {
                        kprintf_putchar_hook_r[0]('0');
                    } else {
                        printfloat_A0A0(f);
                    }
                } break;
                case 's' - '0': {
                    char *string = (ap += 8, *(char **)(ap - 8));
                    if (*string == 0) {
                        kprintf_putchar_hook_r[0]('(');
                        kprintf_putchar_hook_r[0]('n');
                        kprintf_putchar_hook_r[0]('u');
                        kprintf_putchar_hook_r[0]('l');
                        kprintf_putchar_hook_r[0]('l');
                        kprintf_putchar_hook_r[0](')');
                    } else {
                        p = string;
                        if (*p != 0) {
                            do {
                                kprintf_putchar_hook_r[0](*p++);
                            } while (*p != 0);
                        }
                    }
                    break;
                }
                case 'c' - '0':
                    v = (ap += 8, *(char *)(ap - 8));
                    kprintf_putchar_hook_r[0](v);
                    break;
                default:
                    break;
            }
        } else {
            kprintf_putchar_hook_r[0](*fmt);
        }
    }
    if (ie != 0) {
        EnableInterrupts_A0A0();
    }
}

/* The print core: walks fmt with the va_list, writing through the hook
   kprintf_putchar_hook, and returns the count. */
extern int kprintf_format(const char *, va_list);

/*
 * A real varargs forwarder: va_start, hand fmt and the va_list to
 * kprintf_format, va_end. Its frame is printf's (func_00116078, also
 * 2.9-ee): $ra at 0 and the $5-$11 save area at 0x58, which is where ap
 * points. Exact under 2.9-ee; under the game's 2.95.3 a varargs
 * definition also spills $f12-$f18 (SIZE 72/56), which is why this was
 * once a hand-built `long args[16]` forwarder (13/56).
 */
void kprintf(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    kprintf_format(fmt, ap);
    va_end(ap);
}

/*
 * The varargs twin of the function above (likely scePrintf): swap the
 * print hook kprintf_putchar_hook for deci2Putchar (the deci2 console writer),
 * pass fmt and the va_list to kprintf_format, restore the hook. Exact
 * under 2.9-ee; under 2.95.3 the frame comes out 0x90 against retail's
 * 0xB0 (the $f12-$f18 save again, as with sprintf, 00116248.c).
 */
extern void deci2Putchar();

int scePrintf(const char *fmt, ...) {
    va_list ap;
    void *saved = kprintf_putchar_hook;
    int ret;

    kprintf_putchar_hook = (void *)deci2Putchar;
    va_start(ap, fmt);
    ret = kprintf_format(fmt, ap);
    va_end(ap);
    kprintf_putchar_hook = saved;
    return ret;
}

void sif_cmd_set_sreg_handler(void *arg0, void *arg1) {
    int idx = *(int *)((char *)arg0 + 0x10);
    int val = *(int *)((char *)arg0 + 0x14);
    int *arr = *(int **)((char *)arg1 + 0x1C);
    arr[idx] = val;
}

int sif_cmd_change_addr_handler(void *arg0, void *arg1) {
    int v = *(int *)((char *)arg0 + 0x10);
    *(int *)((char *)arg1 + 0x8) = v;
    return v;
}

extern int sif_sregs[];

int sceSifGetSreg(int arg0) {
    return sif_sregs[arg0];
}

LINKER_REMNANT("asm/remnants/core_text", func_0011A770);

extern int sif_cmd_init_done;
extern int sif_cmd_dmac_handler_id;
typedef struct {
    void (*f)(void *, void *);
    void *arg;
} Hnd;
typedef struct {
    void *a0;
    void *a1;
    int a2;
    void *a3;
    int a4;
    int a5;
    int a6;
    int *a7;
} CmdData;
typedef struct {
    char pad[0xC];
    int pad2;
    void *u10;
} CmdPkt;
extern int disable_interrupts(void);
extern void enable_interrupts(void);
extern void flush_cache(int);
extern void sce_sif_set_dchain(void);
extern int add_dmac_handler(int, void *, int);
extern void enable_dmac_atomic(int);
extern void sif_cmd_intr_handler(void);
extern unsigned char sif_cmd_receive_buffer[];
extern unsigned char sif_cmd_send_buffer[];
extern CmdData sif_cmd_data;
extern Hnd sif_system_handlers[];
extern CmdPkt sif_init_packet;

void sceSifInitCmd(void) {
    int i;
    unsigned t5, t6;
    disable_interrupts();
    if (sif_cmd_init_done != 0) {
        enable_interrupts();
        return;
    }
    sif_cmd_init_done = 1;
    t6 = (unsigned)sif_cmd_receive_buffer | 0x20000000;
    t5 = (unsigned)sif_cmd_send_buffer | 0x20000000;
    sif_cmd_data.a0 = (void *)t6;
    sif_cmd_data.a1 = (void *)t5;
    sif_cmd_data.a2 = 0;
    sif_cmd_data.a3 = sif_system_handlers;
    sif_cmd_data.a4 = 0x20;
    sif_cmd_data.a5 = 0;
    sif_cmd_data.a6 = 0;
    sif_cmd_data.a7 = sif_sregs;
    for (i = 0; i < 32; i++) {
        sif_system_handlers[i].f = 0;
        sif_system_handlers[i].arg = 0;
    }
    for (i = 0; i < 32; i++) {
        sif_sregs[i] = 0;
    }
    sif_system_handlers[0].f = (void (*)(void *, void *))sif_cmd_change_addr_handler;
    sif_system_handlers[1].f = (void (*)(void *, void *))sif_cmd_set_sreg_handler;
    sif_system_handlers[0].arg = &sif_cmd_data;
    sif_system_handlers[1].arg = &sif_cmd_data;
    enable_interrupts();
    flush_cache(0);
    if (*(volatile unsigned *)0x1000E010 & 0x20) {
        *(volatile unsigned *)0x1000E010 = 0x20;
    }
    if (!(*(volatile unsigned *)0x1000C000 & 0x100)) {
        sce_sif_set_dchain();
    }
    sif_cmd_dmac_handler_id = add_dmac_handler(5, sif_cmd_intr_handler, 0);
    enable_dmac_atomic(5);
    sif_cmd_data.a2 = ((int (*)(unsigned))sce_sif_get_reg)(0x80000000);
    if (sif_cmd_data.a2 != 0) {
        sif_init_packet.u10 = sif_cmd_receive_buffer;
        sceSifSendCmd(0x80000000, (int)&sif_init_packet, 0x14, 0, 0, 0);
        return;
    }
    do {
    } while (!(sce_sif_get_reg(4) & 0x20000));
    sif_cmd_data.a2 = sce_sif_get_reg(2);
    sce_sif_set_reg(0x80000000, sif_cmd_data.a2);
    sce_sif_set_reg(0x80000001, (int)&sif_cmd_data);
    sif_init_packet.u10 = sif_cmd_receive_buffer;
    sif_init_packet.pad2 = 0;
    sceSifSendCmd(0x80000002, (int)&sif_init_packet, 0x14, 0, 0, 0);
}

extern int sif_cmd_dmac_handler_id;
extern int sif_cmd_init_done;

extern int disable_dmac_atomic(int);
extern int remove_dmac_handler(int, int);

/* disable_dmac_atomic and remove_dmac_handler both return int; their results decide
   which registers the globals' addresses get. */
void sceSifExitCmd(void) {
    disable_dmac_atomic(0x5);
    remove_dmac_handler(0x5, sif_cmd_dmac_handler_id);
    sif_cmd_init_done = 0;
}

extern int sif_cmd_data_sys_handlers NOT_SDA;
extern int sif_cmd_data_usr_handlers NOT_SDA;

/* Sets slot arg0's two words: sif_cmd_data_usr_handlers's for arg0 >= 0, sif_cmd_data_sys_handlers's
   below. In sceSifRemoveCmdHandler's shape: reassigning arg0 to the table is what
   puts the base in $a0, as in retail. */
void sceSifAddCmdHandler(int arg0, int arg1, int arg2) {
    int off = arg0 << 3;
    if (arg0 < 0) {
        arg0 = sif_cmd_data_sys_handlers;
    } else {
        arg0 = sif_cmd_data_usr_handlers;
    }
    off += arg0;
    *(int *)(off + 0) = arg1;
    *(int *)(off + 4) = arg2;
}

void sceSifRemoveCmdHandler(int arg0) {
    int offset = arg0 << 3;
    if (arg0 < 0) {
        arg0 = sif_cmd_data_sys_handlers;
    } else {
        arg0 = sif_cmd_data_usr_handlers;
    }
    offset += arg0;
    *(int *)offset = 0;
}

typedef struct {
    u32 f0 : 8;
    u32 f0_8 : 24;
    u32 f4;
    u32 f8;
} SifCmd;
typedef struct {
    u32 f0;
    u32 f4;
    u32 f8;
    u32 fC;
} SifDma;
extern u32 sif_command_dma_destination[];
extern void sceSifWriteBackDCache(void *addr, s32 size);
extern s32 sce_sif_set_dma();
extern s32 i_sce_sif_set_dma();

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/sdk/rpc/sce_sif_send_cmd.c, _sceSifSendCmd. */
s32 sif_send_cmd_common(u32 packet, s32 mode, SifCmd *cmd, s32 size, u32 src, u32 dst, s32 dst_size) {
    SifDma tags[2];
    s32 num;
    register u32 mode44;
    register u32 addr0;

    if ((u32)(size - 0x10) >= 0x61) {
        return 0;
    }
    num = 0;
    if (dst_size > 0) {
        cmd->f0_8 = dst_size;
        tags[0].f0 = src;
        tags[0].f4 = dst;
        cmd->f4 = dst;
        tags[0].f8 = dst_size;
        tags[0].fC = 0;
        num = 1;
        if (mode & 4) {
            sceSifWriteBackDCache(src, dst_size);
        }
    } else {
        cmd->f4 = 0;
        cmd->f0_8 = 0;
    }
    addr0 = sif_command_dma_destination[0];
    tags[num].f0 = (u32)cmd;
    tags[num].f4 = addr0;
    tags[num].f8 = size;
    cmd->f0 = size;
    cmd->f8 = packet;
    tags[num].fC = 0x44;
    sceSifWriteBackDCache(cmd, size);
    num++;
    if (mode & 1) {
        return i_sce_sif_set_dma(tags, num);
    }
    return sce_sif_set_dma(tags, num);
}

/* sceSifSendCmd and isceSifSendCmd: forward to the common sender
   sif_send_cmd_common with the mode (0, or 1 from interrupt context) injected
   as its second argument. EABI passes the first eight integer args in
   $4-$11, so this forwards seven of them.

   Both return the sender's result (it sets $v0 on every path; the packet
   id). That is what keeps 2.9-ee from turning the call into a bare tail
   jump (SIZE 40/60 when void), and its schedule is then retail's: the
   `addiu $sp,$sp,-0x10` fourth, after three argument moves, which
   2.95.3 could not produce (11/60). */

int sceSifSendCmd(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5) {
    return sif_send_cmd_common(arg0, 0, arg1, arg2, arg3, arg4, arg5);
}

int isceSifSendCmd(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5) {
    return sif_send_cmd_common(arg0, 0x1, arg1, arg2, arg3, arg4, arg5);
}

ASM_FUNC("asm/handwritten/core_text", func_0011AC48);

ASM_FUNC("asm/handwritten/core_text", func_0011AD70);

/*
 * The last word of sceSifWriteBackDCache (sceSifWriteBackDCache, hand-written): a bare
 * `jr $31` whose delay slot is the next object's first instruction. The SDK's
 * libkernl.a gives the function 176 bytes in sifcmd.o (0x11AD70..0x11AE20),
 * this word included, after its own `last:` label's `jr $31; nop`
 * (docs/reference/ASM_CLASSIFICATION.md).
 */
ASM_FUNC("asm/handwritten/core_text", func_0011AE1C);

typedef struct {
    int pid;
    void *pkt_table;
    int pkt_table_len;
    int unk0C;
    int unk10;
    void *rdata_table;
    int rdata_table_len;
    void *client_table;
    int client_table_len;
    int rdata_table_idx;
} SifRpcData;

extern int RpcCommandState NOT_SDA;
extern char sif_rpc_request_packets[];
extern char sif_rpc_reply_packets[];
extern char sif_rpc_client_table[];
extern SifRpcData sif_rpc_data_d __asm__("D_00156900");
extern int disable_interrupts(void); /* DI */
extern void enable_interrupts(void); /* EI */
extern void sceSifInitCmd(void); /* sceSifInitCmd */

extern int sce_sif_set_reg(int, int); /* sceSifSetReg */
extern void sif_rpc_request_end(char *);
extern void sif_rpc_request_bind(void *, void *);
extern void sif_rpc_request_call(void *, void *);
extern void sif_rpc_request_rdata(void *, void *);

/* sceSifInitRpc: once, set up the RPC tables and command handlers, then
   tell the IOP (SIF register 0x80000002) and wait for it. Adapted from
   Lombyte's project (GPL-3.0) for PAL. */
void sceSifInitRpc(int mode) {
    int *packet;

    disable_interrupts();
    if (RpcCommandState) {
        enable_interrupts();
        return;
    }
    RpcCommandState = 1;
    enable_interrupts();
    sceSifInitCmd();
    disable_interrupts();
    sif_rpc_data_d.pkt_table = (void *)((unsigned int)sif_rpc_request_packets | 0x20000000);
    sif_rpc_data_d.pkt_table_len = 32;
    sif_rpc_data_d.unk0C = 0;
    sif_rpc_data_d.unk10 = 0;
    sif_rpc_data_d.rdata_table = (void *)((unsigned int)sif_rpc_reply_packets | 0x20000000);
    sif_rpc_data_d.rdata_table_len = 32;
    sif_rpc_data_d.client_table = (void *)((unsigned int)sif_rpc_client_table | 0x20000000);
    sif_rpc_data_d.client_table_len = 32;
    sif_rpc_data_d.rdata_table_idx = 0;
    sif_rpc_data_d.pid = 1;
    sceSifAddCmdHandler(0x80000008, (int)sif_rpc_request_end, (int)&sif_rpc_data_d);
    sceSifAddCmdHandler(0x80000009, (int)sif_rpc_request_bind, (int)&sif_rpc_data_d);
    sceSifAddCmdHandler(0x8000000A, (int)sif_rpc_request_call, (int)&sif_rpc_data_d);
    sceSifAddCmdHandler(0x8000000C, (int)sif_rpc_request_rdata, (int)&sif_rpc_data_d);
    enable_interrupts();
    if (sce_sif_get_reg(0x80000002)) {
        return;
    }
    packet = (int *)(sif_rpc_request_packets + 64);
    packet[3] = 1;
    sceSifSendCmd(0x80000002, (int)packet, 16, 0, 0, 0);
    while (sceSifGetSreg(0) == 0) {
    }
    sce_sif_set_reg(0x80000002, 1);
}

extern void sceSifExitCmd(void);

void sceSifExitRpc(void) {
    sceSifExitCmd();
    RpcCommandState = 0;
}

extern int disable_interrupts(void);
extern void enable_interrupts(void);

/* Claim the first free 0x40-byte slot in the table {next_id, base,
   count} at arg0, with interrupts held off across the search. A claimed
   slot records its index and state in +0x10, points +0x14 at itself and
   takes the next id; id 0 is skipped, so after handing out 1 the
   counter jumps straight to 2. Returns the slot, or 0 if the table is
   full.

   Two shapes mattered. `i` must be assigned AFTER the disable_interrupts
   call: initialised in its declaration it is live across the call, so
   it lands in a third callee-saved register and drags in a save/restore
   pair, where retail keeps it in $v1.

   And the id fan-in has to be written as an explicit if/ELSE with a
   separate assignment in each arm. Retail rematerialises `addiu $v1,$0,1`
   in the taken arm even though $v1 already holds that value, and no
   form that computes the value once -- including writing the redundant
   `id = 1;` after the store -- survives: the compiler folds it away and
   the function comes out 4 bytes short. Giving the two arms their own
   assignments to `id` keeps both definitions alive. */
void *get_available_rpc_packet(void *arg0) {
    char *a = (char *)arg0;
    int n;
    char *p;
    int i;
    int id;
    int v;

    disable_interrupts();
    n = *(int *)(a + 0x8);
    i = 0;
    p = *(char **)(a + 0x4);
    while (i < n) {
        if ((*(int *)(p + 0x10) & 1) == 0) {
            *(int *)(p + 0x10) = (i << 16) | 5;
            v = *(int *)a + 1;
            *(int *)a = v;
            if (v == 1) {
                *(int *)a = v + 1;
                id = 1;
            } else {
                id = v;
            }
            *(char **)(p + 0x14) = p;
            *(int *)(p + 0x18) = id;
            enable_interrupts();
            return p;
        }
        i++;
        p += 0x40;
    }
    enable_interrupts();
    return 0;
}

void sif_rpc_free_packet(void *arg0) {
    char *self = (char *)arg0;
    unsigned int flags = *(unsigned int *)(self + 0x10);
    *(int *)(self + 0x18) = 0;
    *(unsigned int *)(self + 0x10) = flags & 0xFFFFFFFEu;
}

/* Hand out the next 0x40-byte slot of the ring {.., +0x14 base, +0x18
   count, .., +0x24 next}: take next modulo count, advance next past it,
   return the slot. The slot address has to be formed BEFORE the store of
   the new index: that is what gives retail's allocation (remainder in $v0,
   divisor in $v1) and its tail, `addiu` before `sll` and the `addu` in the
   jr delay slot. Storing first and indexing in the return is 20/48. */
/* The slot address is computed into a local before next = i + 1 is
   stored. */
void *sif_rpc_get_fpacket(void *arg0) {
    char *p = (char *)arg0;
    int i = *(int *)(p + 0x24) % *(int *)(p + 0x18);
    char *s = *(char **)(p + 0x14) + i * 0x40;
    *(int *)(p + 0x24) = i + 1;
    return s;
}

/*
 * sifrpc's _request_end: the completion handler for a request block. Two
 * command ids do extra work before the common teardown: 0x8000000A (RPC
 * call) runs the client's end callback, 0x80000009 (RPC bind) copies the
 * server, buff and cbuff back into it. Then, whatever the id was,
 * iSignalSema the client's semaphore if it has one, free its packet
 * (sif_rpc_free_packet) and clear the pointer. Exact under both compilers.
 *
 * Two levers (build-sn/try/sif_rpc_request_end/RESULT.md):
 *  - Retail's dispatch is gcc's case-node decision tree rooted at
 *    0x8000000A, which a two-case switch never builds (at two nodes it
 *    stays a chain). The switch has a third case above 0x8000000A that
 *    shares the default's body: with one shared label the node stays in
 *    the case list, the three-node tree roots at the median, and the
 *    now-redundant test is deleted. Any value above 0x8000000A works;
 *    0x8000000C is ps2sdk's SIF_CMD_RPC_RDATA, a guess.
 *  - `o` is loaded inside each arm and reloaded only after the callback.
 *    (Re-read once after the switch it is also exact under 2.9-ee, whose
 *    -fstrict-aliasing drops the reload after the 0x80000009 stores, but
 *    2.95.3 keeps that reload.)
 */
extern int i_signal_semaphore(int);

void sif_rpc_request_end(char *p) {
    char *o;
    int h;

    switch (*(unsigned int *)(p + 0x20)) {
        case 0x8000000A:
            o = *(char **)(p + 0x1C);
            if (*(int *)(o + 0x1C) != 0) {
                (*(void (**)(int))(o + 0x1C))(*(int *)(o + 0x20));
                o = *(char **)(p + 0x1C);
            }
            break;
        case 0x80000009:
            o = *(char **)(p + 0x1C);
            *(int *)(o + 0x24) = *(int *)(p + 0x24);
            *(int *)(o + 0x14) = *(int *)(p + 0x28);
            *(int *)(o + 0x18) = *(int *)(p + 0x2C);
            break;
        case 0x8000000C:
        default:
            o = *(char **)(p + 0x1C);
            break;
    }
    h = *(int *)(o + 0x8);
    if (h >= 0) {
        i_signal_semaphore(h);
    }
    sif_rpc_free_packet(*(void **)o);
    *(int *)o = 0;
}

/* _request_rdata (libkernl.a:sifrpc.o): the sifcmd system handler for
   SIF_CMD_RPC_RDATA (0x8000000c). Grabs a free response packet
   (sif_rpc_get_fpacket = _rpc_get_fpacket), copies the requester's pkt_addr
   and recvbuf pointer (rdata's words 5 and 7) into the response, tags
   the response SIF_CMD_RPC_RDATA (word 8), then forwards the
   requester's src/dest/size (words 8/9/10) to the IOP as
   isceSifSendCmd(SIF_CMD_RPC_END, ...) (isceSifSendCmd). Void: the
   trailing call is a tail jump. Both copied fields have to be read into
   their own locals before either store: that is what puts pkt_addr in
   $a1 and recvbuf in $a0, matching retail (any other shape -- inline
   reads, one field hoisted, or reversed declaration order -- swaps that
   allocation or reloads recvbuf late). */
void sif_rpc_request_rdata(void *rdata, void *data) {
    int *r = (int *)rdata;
    int *rend = (int *)sif_rpc_get_fpacket(data);
    int pkt_addr = r[5];
    int recvbuf = r[7];

    rend[5] = pkt_addr;
    rend[7] = recvbuf;
    rend[8] = 0x8000000C;
    isceSifSendCmd(0x80000008, (int)rend, 0x40, r[8], r[9], r[10]);
}

void *sif_rpc_search_svdata(int key, void *arg1) {
    void *outer = *(void **)((char *)arg1 + 0x28);
    while (outer != 0) {
        void *inner = *(void **)((char *)outer + 8);
        while (inner != 0) {
            if (*(int *)inner == key) {
                return inner;
            }
            inner = *(void **)((char *)inner + 0x38);
        }
        outer = *(void **)((char *)outer + 0x14);
    }
    return 0;
}

/* _request_bind (libkernl.a:sifrpc.o): the sifcmd system handler for
   SIF_CMD_RPC_BIND (0x80000009). Grabs a free response packet
   (sif_rpc_get_fpacket = _rpc_get_fpacket), copies the requester's pkt_addr
   and cd (bind's words 5 and 7) into it, tags it SIF_CMD_RPC_BIND,
   looks up the requested service id (bind's word 8, search_svdata =
   sif_rpc_search_svdata) and fills in the server/buf/cbuf fields (words 9-11)
   from it if found, zeroing them otherwise, then forwards to the IOP as
   isceSifSendCmd(SIF_CMD_RPC_END, ...) (isceSifSendCmd). Matches
   ps2sdk's _request_bind field-for-field (sifrpc.c).

   Two shapes mattered: pkt_addr and cd need their own locals declared
   in that order (pkt_addr first) even though cd is stored first --
   reversed, the two loads swap registers (sif_rpc_request_rdata's lever again);
   and the not-found case has to be the `if` arm, sd-found the `else`
   (retail branches on `sd == 0` being false, i.e. `bnel`; writing the
   found case first inverts the branch, as in sif_rpc_request_call). */
void sif_rpc_request_bind(void *bind, void *data) {
    int *b = (int *)bind;
    int *rend = (int *)sif_rpc_get_fpacket(data);
    int pkt_addr = b[5];
    int cd = b[7];
    int *sd;

    rend[7] = cd;
    rend[5] = pkt_addr;
    rend[8] = 0x80000009;
    sd = (int *)sif_rpc_search_svdata(b[8], data);
    if (sd == 0) {
        rend[9] = 0;
        rend[10] = 0;
        rend[11] = 0;
    } else {
        rend[9] = (int)sd;
        rend[10] = sd[2];
        rend[11] = sd[5];
    }
    isceSifSendCmd(0x80000008, (int)rend, 0x40, 0, 0, 0);
}

typedef struct {
    int unk0;
    int unk4;
    int unk8;
    char pad_C[0x4];
    int unk10;
    char pad_14[0x10];
    int unk24;
} BindClient;

typedef struct {
    char pad_0[0x14];
    int unk14;
    int unk18;
    int unk1C;
    int unk20;
} BindPacket;

extern char sif_rpc_data[];
extern int create_semaphore(void *); /* CreateSema */

extern int wait_semaphore(int); /* WaitSema */

/* sceSifBindRpc. Adapted from Lombyte's project (GPL-3.0) for PAL. */
int sceSifBindRpc(void *client, int arg1, int arg2) {
    BindClient *arg0 = client;
    int sema_param[6];
    int temp_2_31;
    BindPacket *temp_2_15;

    arg0->unk10 = 0;
    arg0->unk24 = 0;
    temp_2_15 = (BindPacket *)get_available_rpc_packet(sif_rpc_data);
    if (temp_2_15 == 0) {
        return -1;
    }
    arg0->unk4 = temp_2_15->unk18;
    arg0->unk0 = (int)temp_2_15;
    temp_2_15->unk20 = arg1;
    temp_2_15->unk14 = (int)temp_2_15;
    temp_2_15->unk1C = (int)arg0;
    if (arg2 & 1) {
        goto block_7;
    }
    sema_param[1] = 1;
    sema_param[2] = 0;
    temp_2_31 = create_semaphore(sema_param);
    arg0->unk8 = temp_2_31;
    if (temp_2_31 >= 0) {
        goto block_4;
    }
    sif_rpc_free_packet(temp_2_15);
    return -3;
block_4:
    if (sceSifSendCmd(0x80000009, (int)temp_2_15, 0x40, 0, 0, 0) != 0) {
        goto block_6;
    }
    sif_rpc_free_packet(temp_2_15);
    delete_semaphore(arg0->unk8);
    return -2;
block_6:
    wait_semaphore(arg0->unk8);
    delete_semaphore(arg0->unk8);
    return 0;
block_7:
    arg0->unk8 = -1;
    if (sceSifSendCmd(0x80000009, (int)temp_2_15, 0x40, 0, 0, 0) != 0) {
        return 0;
    }
    sif_rpc_free_packet(temp_2_15);
    return -2;
}

extern void iWakeupThread(int thread_id);

/* _request_call (libkernl.a:sifrpc.o): the sifcmd system handler for
   SIF_CMD_RPC_CALL (0x8000000a). Queues the request's server object
   (request->sd, word 13) onto its data queue's (sd->base, word 16)
   linked list of pending calls -- append to the current end if the
   queue is non-empty (base->start, word 3), else start a new list --
   then copies the request's fields into the server object (pkt_addr,
   client/cd, rpc_number, size/send_size, recvbuf, rsize/recv_size,
   rmode, rid/rec_id), and finally wakes the queue's worker thread
   (iWakeupThread, iWakeupThread) unless it is already running or has
   none. Matches ps2sdk's _request_call field-for-field (sifrpc.c).

   Three shapes mattered: the empty-queue arm has to come first (retail
   tests and branches on it, not on the append case); and the first two
   copied fields (pkt_addr, client) need their own locals, both declared
   right there (not hoisted to the top, which pulls their loads into the
   queue-splice code) and both declared before either store -- the same
   allocator lever as sif_rpc_request_rdata -- or the pair's load/store order
   swaps or its two registers trade places. */
void sif_rpc_request_call(void *request, void *data) {
    int *req = (int *)request;
    int *sd = (int *)req[13];
    int *base = (int *)sd[16];

    if (base[3] == 0) {
        base[3] = (int)sd;
    } else {
        int *end = (int *)base[4];
        end[15] = (int)sd;
    }
    base[4] = (int)sd;
    {
        int pkt_addr = req[5];
        int client = req[7];
        sd[8] = pkt_addr;
        sd[7] = client;
    }
    sd[9] = req[8];
    sd[3] = req[9];
    sd[10] = req[10];
    sd[11] = req[11];
    sd[12] = req[12];
    sd[13] = req[4];

    if (base[0] < 0 || base[1] != 0)
        return;
    iWakeupThread(base[0]);
}
struct SifRpcClient {
    volatile s32 packet;
    volatile s32 request_id;
    s32 semaphore_id;
    u8 pad_C[0x8];
    s32 server_buffer;
    u8 pad_18[0x4];
    s32 end_callback;
    volatile s32 end_argument;
    s32 server;
};
struct SifRpcPacket {
    u8 pad_0[0x14];
    s32 packet_address;
    s32 request_id;
    s32 client;
    s32 rpc_number;
    s32 send_size;
    s32 receive_address;
    s32 receive_size;
    s32 completion_mode;
    s32 server;
};
extern u8 sif_rpc_data_r[] __asm__("D_00156900");
extern s32 create_semaphore();
extern s32 delete_semaphore_r() __asm__("func_00118C80");
extern s32 wait_semaphore();
extern s32 get_available_rpc_packet_r() __asm__("func_0011AFE8");
extern s32 sif_rpc_free_packet_r() __asm__("func_0011B090");
extern s32 sceSifSendCmd();
extern s32 sceSifWriteBackDCache_r() __asm__("func_0011AD70");

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/sdk/rpc/sce_sif_call_rpc.c, sceSifCallRpc. */
extern s32 func_0011B4C8_r(struct SifRpcClient *client, s32 rpc_number, s32 mode, s32 send_buf, s32 send_size,
                           s32 recv_buf, s32 recv_size, s32 end_func, s32 end_data) __asm__("func_0011B4C8");
s32 func_0011B4C8_r(struct SifRpcClient *client, s32 rpc_number, s32 mode, s32 send_buf, s32 send_size, s32 recv_buf,
                    s32 recv_size, s32 end_func, s32 end_data) {
    s32 semaphore_parameters[6];
    struct SifRpcPacket *packet;
    s32 semaphore_id;
    s32 skip_cache_writeback;
    s32 request_id;
    s32 allocation_failure_result;
    packet = (struct SifRpcPacket *)get_available_rpc_packet_r(sif_rpc_data_r);
    allocation_failure_result = -1;
    if (packet == 0) {
        return allocation_failure_result;
    }
    request_id = packet->request_id;
    skip_cache_writeback = mode & 2;
    client->end_argument = end_data;
    client->packet = (s32)packet;
    client->request_id = request_id;
    client->end_callback = end_func;
    packet->rpc_number = rpc_number;
    packet->send_size = send_size;
    packet->receive_address = recv_buf;
    packet->receive_size = recv_size;
    packet->packet_address = (s32)packet;
    packet->server = client->server;
    packet->client = (s32)client;
    if (!skip_cache_writeback) {
        if (send_buf == recv_buf) {
            sceSifWriteBackDCache_r(send_buf, (send_size >= recv_size) ? (send_size) : (recv_size));
        } else {
            if (send_size > 0) {
                sceSifWriteBackDCache_r(send_buf, send_size);
            }
            if (recv_size > 0) {
                sceSifWriteBackDCache_r(recv_buf, recv_size);
            }
        }
    }
    if (mode & 1) {
        if (end_func == 0) {
            packet->completion_mode = 0;
        } else {
            packet->completion_mode = 1;
        }
        client->semaphore_id = -1;
        if (sceSifSendCmd(0x8000000A, packet, 0x40, send_buf, client->server_buffer, send_size) != 0) {
            return 0;
        } else {
            sif_rpc_free_packet_r(packet);
            return -2;
        }
    } else {
        semaphore_parameters[1] = 1;
        semaphore_parameters[2] = 0;
        semaphore_id = create_semaphore(semaphore_parameters);
        client->semaphore_id = semaphore_id;
        if (semaphore_id < 0) {
            sif_rpc_free_packet_r(packet);
            return -3;
        }
        packet->completion_mode = 1;
        if (sceSifSendCmd(0x8000000A, packet, 0x40, send_buf, client->server_buffer, send_size) == 0) {
            delete_semaphore_r(client->semaphore_id);
            sif_rpc_free_packet_r(packet);
            return -2;
        }
        wait_semaphore(client->semaphore_id);
        delete_semaphore_r(client->semaphore_id);
        return 0;
    }
}

/* Is the handle {slot, id} at arg0 still live: the slot is set, its id at
   +0x18 still matches the handle's, and its in-use bit (+0x10 bit 0) is
   set. Written as one inverted guard that returns 0, then `return 1`:
   that keeps retail's two exits (shared `return 0` block the tests fall
   into, a separate `return 1`), and gcc leaves all three branch slots to
   the assembler, which fills them with nops as retail has them. The &&
   form returning 1 merges the exits; early returns give bnel. */
/* One test for the three failure cases, then return 1. */
int sce_sif_check_stat_rpc(void *arg0) {
    char *p = (char *)arg0;
    char *q = *(char **)p;

    if (q == 0 || *(int *)(p + 4) != *(int *)(q + 0x18) || !(*(int *)(q + 0x10) & 1)) {
        return 0;
    }
    return 1;
}

LINKER_REMNANT("asm/remnants/core_text", func_0011B6F8);

extern int create_semaphore(void *);
extern int fio_iob_sema;
extern int fio_async_table_sema;

/*
 * One-shot init: if fio_iob_sema is still -1, fill a semaphore descriptor
 * (the kernel's SemaParam: initCount [2] = 1, maxCount [1] = 1, option
 * [5] = 0) and create two semaphores from it via create_semaphore
 * (CreateSema), storing them into fio_iob_sema and fio_async_table_sema.
 *
 * Under 2.9-ee the store order decides which store lands in the first
 * call's delay slot: written initCount, maxCount, option it is exact
 * (retail puts buf[2] in the slot). Under the game's 2.95.3 no order was
 * exact (8/92 at best), which is what kept this one open.
 */
void fs_iob_sema_make(void) {
    int buf[8];
    if (fio_iob_sema == -1) {
        buf[2] = 1;
        buf[1] = 1;
        buf[5] = 0;
        fio_iob_sema = create_semaphore(buf);
        fio_async_table_sema = create_semaphore(buf);
    }
}

extern char fio_iob[];

/*
 * Claim the first free slot of the 32-entry, 0x10-byte table fio_iob
 * -- the table fs_get_iob below indexes -- under the
 * wait_semaphore/signal_semaphore lock (WaitSema/SignalSema). A slot is free
 * when its +4 word is zero; claiming it writes 0x10000000 there and
 * returns the slot, and a full table returns 0. Both exits drop the lock
 * first.
 *
 * Exact under 2.9-ee with the C that 2.95.3 compiled one instruction
 * short: retail spends a third callee-saved register ($17 holds
 * %hi(fio_iob_sema) for the whole function), which is 2.9-ee's allocation;
 * 2.95.3 parked the %hi in $4 instead.
 */
void *fs_new_iob(void) {
    char *p;

    fs_iob_sema_make();
    wait_semaphore(fio_iob_sema);
    for (p = fio_iob; p < fio_iob + 0x200; p += 0x10) {
        if (*(int *)(p + 0x4) == 0) {
            *(int *)(p + 0x4) = 0x10000000;
            signal_semaphore(fio_iob_sema);
            return p;
        }
    }
    signal_semaphore(fio_iob_sema);
    return 0;
}

/* The out-of-range guard comes first and returns 0: 2.9-ee then branches
   to the in-range block on the true side of the sltiu, as retail does
   (bnez). The in-range-first if/else that matched under 2.95.3 is laid
   out the other way round by 2.9-ee. */
void *fs_get_iob(unsigned int arg0) {
    void *ret;
    fs_iob_sema_make();
    wait_semaphore(fio_iob_sema);
    if (arg0 >= 0x20) {
        signal_semaphore(fio_iob_sema);
        return 0;
    }
    ret = &fio_iob[arg0 * 0x10];
    signal_semaphore(fio_iob_sema);
    return ret;
}

typedef struct {
    s32 v;
} __attribute__((packed)) Word;

typedef struct {
    Word ret;
    Word func;
    Word dest;
    Word size;
} FsRcvHead;

typedef struct {
    void *dest;
} __attribute__((packed)) FsRcvPtr;

typedef struct {
    Word dest;
    Word size;
} FsRcvBuf;

typedef struct {
    s32 n1;
    s32 n2;
    u8 *d1;
    u8 *d2;
    u8 buf1[0x40];
    u8 buf2[0x40];
} FsRcvRead;

typedef struct {
    u8 b[0x144];
} __attribute__((packed)) FsDirent;

typedef struct {
    u8 b[0x40];
} __attribute__((packed)) FsStat;

extern volatile s32 fio_async_semas[];
extern u32 sif_file_completion_bank_index[];
extern s32 fs_double_buffered[];
extern u8 fs_receive_buffers[];
extern void *libc_memcpy(void *, const void *, u32);

void fs_receive_handler(s32 *arg) {
    FsRcvHead h;
    FsRcvHead *src;
    FsRcvBuf *bs;
    union {
        FsRcvPtr p;
        FsRcvBuf b;
    } u;
    FsRcvRead *r;
    u8 *pkt;
    u8 *base;
    u8 *d;
    u32 *bank = sif_file_completion_bank_index;
    s32 idx;
    s32 i;
    s64 ret;
    u32 n;
    u32 addr;

    idx = fs_double_buffered[0] != 0 ? arg[3] : 0;
    *bank = idx;
    base = fs_receive_buffers;
    addr = (u32)base;
    addr += (u32)idx * 0x440;
    pkt = (u8 *)(addr | 0x20000000);
    src = (FsRcvHead *)pkt;
    h.ret = src->ret;
    h.func = src->func;
    h.dest = src->dest;
    h.size = src->size;
    if (h.ret.v >= 0) {
        libc_memcpy((void *)h.dest.v, pkt + 0x10, h.size.v);
    }
    switch (h.func.v) {
        case 2:
            r = (FsRcvRead *)(pkt + 0x14);
            if (r->n1 > 0) {
                d = r->d1;
                for (i = 0; i < r->n1; i++) {
                    d[i] = r->buf1[i];
                }
            }
            if (r->n2 > 0) {
                d = r->d2;
                for (i = 0; i < r->n2; i++) {
                    d[i] = r->buf2[i];
                }
            }
            break;
        case 11:
            u.p = *(FsRcvPtr *)(pkt + 0x14);
            *(FsDirent *)u.p.dest = *(FsDirent *)(pkt + 0x18);
            break;
        case 12:
            u.p = *(FsRcvPtr *)(pkt + 0x14);
            *(FsStat *)u.p.dest = *(FsStat *)(pkt + 0x18);
            break;
        case 23:
        case 25:
        case 26:
            bs = (FsRcvBuf *)(pkt + 0x14);
            u.b.dest = bs->dest;
            u.b.size = bs->size;
            n = u.b.size.v;
            if (n > 0x400) {
                u.b.size.v = 0x400;
                n = 0x400;
            }
            libc_memcpy((void *)u.b.dest.v, pkt + 0x1C, n);
            break;
    }
    if (h.ret.v < 0) {
        u32 semaphore = 0u - (u32)h.ret.v;
        ret = (s32)semaphore;
        h.ret.v = (s32)semaphore;
        for (i = 0; i < 32; i++) {
            if (((s32 *)fio_async_semas)[i] == ret) {
                fio_async_semas[i] = -1;
                break;
            }
        }
        return;
    }
    i_signal_semaphore(h.ret.v);
}

extern int fio_wait_sema;
extern int create_semaphore(void *);

void fs_call_sema_init(void) {
    int buf[8];
    if (fio_wait_sema == -1) {
        buf[5] = 0;
        buf[2] = 1;
        buf[1] = 1;
        fio_wait_sema = create_semaphore(buf);
    }
}

extern void fs_call_sema_init(void);
extern int fio_wait_sema NOT_SDA;

int fs_call_sema_wait(void) {
    fs_call_sema_init();
    wait_semaphore(fio_wait_sema);
    return 0;
}

/* A tail jump with the argument load in its delay slot (`lui / j /
   lw`), as 2.9-ee, which builds this SDK object, emits it. */
void fs_call_sema_signal(void) {
    signal_semaphore(fio_wait_sema);
}

ASM_FUNC("asm/handwritten/core_text", func_0011BC80);

/* CD/DVD init step: install the two RPC-side handlers, bind the RPC server
 * at 0x80000001 (retrying until it has an address), clear the 32-slot table
 * fio_iob, then send command 0xFF and copy back the reply word and the
 * flag that says whether the reply was 2. Returns 0 on success, -1 when the
 * bind failed, 0xFFFEFFFF when the RPC failed. */
struct RpcCb {
    void (*cb)(void *);
    void *data;
};
extern struct RpcCb fs_user_callback;
extern char fs_receive_interrupt_handler_data[];
extern char fio_rpc_client[];
extern char fio_rpc_result[];
extern char *fs_init_completion_buffer_addresses[];
extern char FsResetBuffer[];
extern int FsResetState;
extern void fs_receive_handler();
extern void fs_user_callback_handler();

struct W4 {
    char b[4];
};
s32 sceFsInit(void) {
    s32 r, i;
    char *p;
    struct W4 w;
    s32 *q;

    struct RpcCb *rq = &fs_user_callback;
    sceSifInitRpc(0);
    fs_user_callback.cb = 0;
    rq->data = 0;
    r = disable_interrupts();
    sceSifAddCmdHandler(0x80000011, (int)fs_receive_handler, (int)fs_receive_interrupt_handler_data);
    sceSifAddCmdHandler(0x80000013, (int)fs_user_callback_handler, (int)rq);
    if (r != 0) {
        enable_interrupts();
    }
    for (;;) {
        r = sceSifBindRpc(fio_rpc_client, 0x80000001, 0);
        if (r < 0) {
            return -1;
        }
        if (*(s32 *)(fio_rpc_client + 0x24) != 0) {
            break;
        }
        for (i = 0x100000; i != -1; i--) {
        }
    }
    fs_iob_sema_make();
    wait_semaphore(fio_iob_sema);
    for (p = fio_iob; p < fio_iob + 0x200; p += 0x10) {
        *(s32 *)(p + 4) = 0;
    }
    signal_semaphore(fio_iob_sema);
    fs_init_completion_buffer_addresses[0] = fs_receive_buffers;
    fs_init_completion_buffer_addresses[1] = fs_receive_buffers + 0x440;
    r = sif_call_rpc(fio_rpc_client, 0xFF, 0, fs_init_completion_buffer_addresses, 8, fio_rpc_result, 8, 0, 0);
    if (r < 0) {
        return (s32)0xFFFEFFFF;
    }
    *(struct W4 *)FsResetBuffer = *(struct W4 *)((u32)fio_rpc_result | 0x20000000);
    w = *(struct W4 *)((u32)(fio_rpc_result + 4) | 0x20000000);
    fs_double_buffered[0] = (*(s32 *)&w == 2);
    FsResetState = 1;
    return 0;
}

extern char FsResetBuffer[];
extern int server_version_record_address;

/* Three-way compare-and-confirm: the slot FsResetBuffer has to agree with
   the scratch buffer sif_lib_version_string, then with the handle server_version_record_address, and
   finally the two of those with each other. Only the last comparison's
   result is reported, as a 0/1.

   The two globals are bound to locals, buffer first, purely to fix the
   order the compiler materialises their %hi halves in: used directly,
   the slot's lui comes first and the function is 4/140. Declaration
   order of the locals decides it. */
int fs_version_mismatch(void) {
    char *buf = sif_lib_version_string;
    char *slot = FsResetBuffer;
    int r = 0;

    if (libc_memcmp(slot, buf, 4) != 0 && libc_memcmp(slot, server_version_record_address, 4) != 0) {
        r = libc_memcmp(buf, server_version_record_address, 4) != 0;
    }
    return r;
}

extern void libc_memset(void *, int, int);
extern int FsResetState;

int sce_fs_reset(void) {
    FsResetState = 0;
    libc_memset(FsResetBuffer, 0, 0x4);
    return 0;
}

typedef char *va_list_1BF80;
struct FsOpenRequest {
    s32 completion_semaphore;
    void *result;
    s32 result_size;
    s32 flags;
    s32 mode;
    char path[0x400];
    s32 slot;
};
struct SemaphoreParameters {
    s32 count;
    s32 max_count;
    s32 init_count;
    s32 wait_threads;
    s32 attr;
    s32 option;
};
struct SifFileSlot {
    s32 fd;
    s32 flags;
    s32 reserved8;
    s32 reservedC;
};
extern s32 FsResetState_r[] __asm__("D_0012FD94");
extern s32 fs_iob_sema_r[] __asm__("D_0012FDA0");
extern struct FsOpenRequest fs_rpc_send_buffer_r __asm__("D_00156980");
extern u8 fs_rpc_receive_buffer_r[] __asm__("D_001575C0");
extern struct SifFileSlot fs_iob_table_r[] __asm__("D_00157E80");
extern u8 fs_rpc_client_r[] __asm__("D_00158080");
extern s32 create_semaphore_r(struct SemaphoreParameters *) __asm__("func_00118C70");
extern s32 delete_semaphore_r2(s32) __asm__("func_00118C80");
extern s32 fs_call_sema_signal_r(void) __asm__("func_0011BC70");
extern s32 signal_semaphore(s32);
extern s32 wait_semaphore(s32);
extern s32 fs_call_sema_wait_r(s32) __asm__("func_0011BC40");
extern s32 fs_version_mismatch(void);
extern struct SifFileSlot *fs_new_iob_r(void) __asm__("func_0011B770");
extern s32 sceFsInit(void);
extern s32 sif_call_rpc(void *, s32, s32, void *, s32, void *, s32, void *, void *);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/sdk/library/sceopen.c, sceOpen. */
s32 sceOpen(const u8 *path, s32 flags, ...) {
    struct SemaphoreParameters semaphore_parameters;
    va_list_1BF80 arguments;
    s32 result;
    s32 path_index;
    s32 slot_mutex;
    s32 mode;
    s32 completion_semaphore;
    s32 slot_index;
    s32 return_value;
    struct SifFileSlot *file_slot;
    struct FsOpenRequest *request;

    request = &fs_rpc_send_buffer_r;
    /* The six remaining EE argument registers occupy eight bytes each. */
    arguments = __builtin_next_arg(flags) - 0x30;
    fs_call_sema_wait_r(0);
    if (FsResetState_r[0] == 0) {
        sceFsInit();
    }
    if (fs_version_mismatch() != 0) {
        fs_call_sema_signal_r();
        return -0x10004;
    }
    file_slot = fs_new_iob_r();
    if (file_slot == 0) {
        fs_call_sema_signal_r();
        return -0x13;
    }
    mode = *(s32 *)arguments;
    for (path_index = 0; path_index < 0x400; path_index++) {
        if ((request->path[path_index] = path[path_index]) == 0) {
            break;
        }
    }
    if (path_index == 0x400) {
        request->path[0x3FF] = 0;
    }
    slot_index = file_slot - fs_iob_table_r;
    request->flags = (s32)(flags & 0x6FFFFFFF);
    request->mode = mode;
    semaphore_parameters.max_count = 1;
    request->slot = slot_index;
    semaphore_parameters.init_count = 0;
    semaphore_parameters.option = 0;
    completion_semaphore = create_semaphore_r(&semaphore_parameters);
    request->result = &result;
    request->completion_semaphore = completion_semaphore;
    request->result_size = 4;
    if (sif_call_rpc(fs_rpc_client_r, 0, 0, &fs_rpc_send_buffer_r, 0x418, fs_rpc_receive_buffer_r, 4, 0, 0) < 0) {
        delete_semaphore_r2(completion_semaphore);
        fs_call_sema_signal_r();
        return -0xB;
    }
    return_value = *(u32 *)((u32)fs_rpc_receive_buffer_r | 0x20000000);
    fs_call_sema_signal_r();
    if (return_value == 0) {
        delete_semaphore_r2(completion_semaphore);
        return -0xB;
    }
    wait_semaphore(completion_semaphore);
    delete_semaphore_r2(completion_semaphore);
    if (result < 0) {
        wait_semaphore(fs_iob_sema_r[0]);
        file_slot->flags = 0;
        signal_semaphore(fs_iob_sema_r[0]);
        return result;
    }
    return_value = slot_index;
    wait_semaphore(fs_iob_sema_r[0]);
    slot_mutex = fs_iob_sema_r[0];
    file_slot->fd = result;
    file_slot->flags = (s32)(file_slot->flags | flags);
    signal_semaphore(slot_mutex);
    return return_value;
}

typedef struct {
    int handle;
    int active;
} CloseIob;

typedef struct {
    int sema;
    int result;
    int size;
    int handle;
    int slot;
} CloseState;

typedef struct {
    int count;
    int maxCount;
    int initialCount;
    int waitThreads;
    int attr;
    int option;
} CloseSema;

extern CloseState fs_rpc_send_buffer;
extern char fio_rpc_result[];
extern char fio_rpc_client[];
extern int wait_semaphore(int);
extern int sif_call_rpc();
extern int fs_call_sema_wait_arg(int) __asm__("func_0011BC40");

/* sceClose: close a SIF file handle, adapted from Lombyte's project (GPL-3.0) for PAL. */
int sceClose(unsigned int fd) {
    CloseSema sema;
    int result;
    int sid;
    int rpcResult;
    CloseIob *iob;
    CloseState *state = &fs_rpc_send_buffer;

    iob = fs_get_iob(fd);
    fs_call_sema_wait_arg(1);
    if (FsResetState == 0) {
        fs_call_sema_signal();
        return -1;
    }
    if (iob == 0 || iob->active == 0) {
        fs_call_sema_signal();
        return -9;
    }
    state->handle = iob->handle;
    sema.maxCount = 1;
    state->slot = ((char *)iob - fio_iob) >> 4;
    sema.initialCount = 0;
    sema.option = 0;
    sid = create_semaphore(&sema);
    state->sema = sid;
    state->result = (int)&result;
    state->size = 4;
    rpcResult = sif_call_rpc(fio_rpc_client, 1, 0, state, 0x14, fio_rpc_result, 4, 0, 0);
    if (rpcResult < 0) {
        delete_semaphore(sid);
        fs_call_sema_signal();
        return -11;
    }
    iob->active = 0;
    rpcResult = *(int *)((unsigned int)fio_rpc_result | 0x20000000);
    fs_call_sema_signal();
    if (rpcResult == 0) {
        delete_semaphore(sid);
        return -11;
    }
    wait_semaphore(sid);
    delete_semaphore(sid);
    if (result < 0) {
        return result;
    }
    return 0;
}

struct FsSeekRequest {
    s32 completion_semaphore;
    void *result;
    s32 result_size;
    s32 fd;
    s32 offset;
    s32 origin;
    s32 slot;
};
struct SemaphoreParameters2 {
    s32 count;
    s32 max_count;
    s32 init_count;
    s32 wait_threads;
    u32 attr;
    u32 option;
};
extern struct FsSeekRequest fs_rpc_send_buffer_r2 __asm__("D_00156980");
extern volatile s32 fio_async_semas[];
extern s32 fs_iob_sema_2_r[] __asm__("D_0012FDA4");
struct SifClient {
    u8 pad[0x28];
};
extern struct SifClient fs_rpc_client_r2 __asm__("D_00158080");
extern struct SifFileSlot *fs_get_iob_r(s32 fd) __asm__("func_0011B7F8");
extern s32 fs_call_sema_wait_r2(s32) __asm__("func_0011BC40");
extern s32 fs_call_sema_signal_r2(void) __asm__("func_0011BC70");
extern s32 create_semaphore_r2(struct SemaphoreParameters2 *) __asm__("func_00118C70");
extern s32 delete_semaphore_r3(s32) __asm__("func_00118C80");

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/sdk/library/sce_lseek.c, sceLseek. */
s32 sceLseek(s32 fd, s32 offset, s32 origin) {
    struct FsSeekRequest *request;
    struct SifFileSlot *file_slot;
    struct SemaphoreParameters2 semaphore_parameters;
    s32 result;
    s32 flags;
    s32 completion_semaphore;
    s32 async_index;
    volatile s32 *async_slot;
    s32 rpc_result;

    request = &fs_rpc_send_buffer_r2;
    file_slot = fs_get_iob_r(fd);
    fs_call_sema_wait_r2(4);
    if (FsResetState_r[0] == 0) {
        fs_call_sema_signal_r2();
        return -1;
    }
    if (file_slot == 0 || (flags = file_slot->flags) == 0) {
        fs_call_sema_signal_r2();
        return -9;
    }
    request->fd = file_slot->fd;
    request->offset = offset;
    request->origin = origin;
    request->slot = file_slot - fs_iob_table_r;
    semaphore_parameters.max_count = 1;
    semaphore_parameters.init_count = 0;
    semaphore_parameters.option = 0;
    completion_semaphore = create_semaphore_r2(&semaphore_parameters);
    request->result = &result;
    request->result_size = 4;
    fs_rpc_send_buffer_r2.completion_semaphore = completion_semaphore;
    if ((s16)flags & 0x8000) {
        wait_semaphore(fs_iob_sema_2_r[0]);
        for (async_index = 0; async_index < 32; async_index++) {
            volatile s32 *async_semaphores = fio_async_semas;

            async_slot = async_semaphores + async_index;
            if (*async_slot == -1) {
                *async_slot = request->completion_semaphore;
                request->completion_semaphore = -request->completion_semaphore;
                break;
            }
        }
        signal_semaphore(fs_iob_sema_2_r[0]);
    }
    if (sif_call_rpc(&fs_rpc_client_r2, 4, 0, &fs_rpc_send_buffer_r2, 0x1C, fs_rpc_receive_buffer_r, 4, 0, 0) < 0) {
        delete_semaphore_r3(completion_semaphore);
        fs_call_sema_signal_r2();
        return -11;
    }
    rpc_result = *(s32 *)((u32)fs_rpc_receive_buffer_r | 0x20000000);
    fs_call_sema_signal_r2();
    if (rpc_result == 0) {
        delete_semaphore_r3(completion_semaphore);
        return -11;
    }
    if (flags & 0x8000) {
        delete_semaphore_r3(completion_semaphore);
        return 0;
    }
    wait_semaphore(completion_semaphore);
    delete_semaphore_r3(completion_semaphore);
    return result;
}

struct FsReadRequest {
    s32 completion_semaphore;
    void *result;
    s32 result_size;
    s32 fd;
    void *buffer;
    s32 length;
    s32 reserved18;
    s32 slot;
};
extern struct FsReadRequest fs_rpc_send_buffer_r3 __asm__("D_00156980");
extern struct SifFileSlot *fs_get_iob_r2(s32 fd) __asm__("func_0011B7F8");
extern s32 fs_call_sema_wait_r3(s32) __asm__("func_0011BC40");
extern s32 fs_call_sema_signal_r3(void) __asm__("func_0011BC70");
extern s32 create_semaphore_r3(struct SemaphoreParameters2 *) __asm__("func_00118C70");
extern s32 delete_semaphore_r4(s32) __asm__("func_00118C80");
extern void sceSifWriteBackDCache(void *, s32);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/sdk/library/sceRead.c, sceRead. */
s32 sceRead(s32 fd, void *buffer, s32 length) {
    struct FsReadRequest *request;
    struct SifFileSlot *file_slot;
    struct SemaphoreParameters2 semaphore_parameters;
    s32 result;
    s32 flags;
    s32 completion_semaphore;
    s32 async_index;
    volatile s32 *async_slot;
    s32 rpc_result;

    request = &fs_rpc_send_buffer_r3;
    file_slot = fs_get_iob_r2(fd);
    fs_call_sema_wait_r3(2);
    if (FsResetState_r[0] == 0) {
        fs_call_sema_signal_r3();
        return -1;
    }
    if (file_slot == 0 || (flags = file_slot->flags) == 0) {
        fs_call_sema_signal_r3();
        return -9;
    }
    request->fd = file_slot->fd;
    semaphore_parameters.max_count = 1;
    request->slot = file_slot - fs_iob_table_r;
    request->buffer = buffer;
    request->length = length;
    semaphore_parameters.init_count = 0;
    semaphore_parameters.option = 0;
    completion_semaphore = create_semaphore_r3(&semaphore_parameters);
    request->result = &result;
    request->result_size = 4;
    fs_rpc_send_buffer_r3.completion_semaphore = completion_semaphore;
    if ((s16)flags & 0x8000) {
        wait_semaphore(fs_iob_sema_2_r[0]);
        for (async_index = 0; async_index < 32; async_index++) {
            volatile s32 *async_semaphores = fio_async_semas;

            async_slot = async_semaphores + async_index;
            if (*async_slot == -1) {
                *async_slot = request->completion_semaphore;
                request->completion_semaphore = -request->completion_semaphore;
                break;
            }
        }
        signal_semaphore(fs_iob_sema_2_r[0]);
    }
    if (!(flags & 0x20000000)) {
        sceSifWriteBackDCache(buffer, length);
    }
    sceSifWriteBackDCache(request, 0x20);
    if (sif_call_rpc(&fs_rpc_client_r2, 2, 0, &fs_rpc_send_buffer_r3, 0x20, fs_rpc_receive_buffer_r, 4, 0, 0) < 0) {
        delete_semaphore_r4(completion_semaphore);
        fs_call_sema_signal_r3();
        return -11;
    }
    rpc_result = *(s32 *)((u32)fs_rpc_receive_buffer_r | 0x20000000);
    fs_call_sema_signal_r3();
    if (rpc_result == 0) {
        delete_semaphore_r4(completion_semaphore);
        return -11;
    }
    if (flags & 0x8000) {
        delete_semaphore_r4(completion_semaphore);
        return 0;
    }
    wait_semaphore(completion_semaphore);
    delete_semaphore_r4(completion_semaphore);
    return result;
}

struct FsWriteRequest {
    s32 completion_semaphore;
    void *result;
    s32 result_size;
    s32 fd;
    void *buffer;
    s32 length;
    s32 prefix_length;
    u8 prefix[0x10];
    s32 slot;
};
extern struct FsWriteRequest fs_rpc_send_buffer_r4 __asm__("D_00156980");
extern struct SifFileSlot *fs_get_iob_r3(s32 fd) __asm__("func_0011B7F8");
extern s32 fs_call_sema_wait_r4(s32) __asm__("func_0011BC40");
extern s32 fs_call_sema_signal_r4(void) __asm__("func_0011BC70");
extern s32 create_semaphore_r4(struct SemaphoreParameters2 *) __asm__("func_00118C70");
extern s32 delete_semaphore_r5(s32) __asm__("func_00118C80");
extern s32 wait_semaphore(s32);
extern void sceSifWriteBackDCache(void *, s32);
extern s32 sif_call_rpc(void *, s32, s32, void *, s32, void *, s32, void *, void *);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/sdk/library/scewrite.c, sceWrite. */
s32 sceWrite(s32 fd, u8 *buffer, s32 length) {
    struct FsWriteRequest *request;
    struct SifFileSlot *file_slot;
    struct SemaphoreParameters2 semaphore_parameters;
    s32 result;
    s32 flags;
    s32 completion_semaphore;
    s32 async_index;
    volatile s32 *async_slot;
    s32 rpc_result;
    s32 prefix_length;
    s32 prefix_index;
    u32 previous_block_address;

    request = &fs_rpc_send_buffer_r4;
    file_slot = fs_get_iob_r3(fd);
    fs_call_sema_wait_r4(3);
    if (FsResetState_r[0] == 0) {
        fs_call_sema_signal_r4();
        return -1;
    }
    if (file_slot == 0 || (flags = file_slot->flags) == 0) {
        fs_call_sema_signal_r4();
        return -9;
    }
    request->fd = file_slot->fd;
    semaphore_parameters.max_count = 1;
    request->slot = file_slot - fs_iob_table_r;
    request->length = length;
    request->buffer = buffer;
    semaphore_parameters.init_count = 0;
    semaphore_parameters.option = 0;
    completion_semaphore = create_semaphore_r4(&semaphore_parameters);
    request->result = &result;
    request->result_size = 4;
    fs_rpc_send_buffer_r4.completion_semaphore = completion_semaphore;
    if ((s16)flags & 0x8000) {
        wait_semaphore(fs_iob_sema_2_r[0]);
        for (async_index = 0; async_index < 32; async_index++) {
            volatile s32 *async_semaphores = fio_async_semas;

            async_slot = async_semaphores + async_index;
            if (*async_slot == -1) {
                *async_slot = request->completion_semaphore;
                request->completion_semaphore = -request->completion_semaphore;
                break;
            }
        }
        signal_semaphore(fs_iob_sema_2_r[0]);
    }
    if (((u32)buffer & 0xF) == 0) {
        prefix_length = 0;
    } else {
        previous_block_address = (u32)buffer - 0x10;
        prefix_length = (((u32)buffer >> 4) << 4) - previous_block_address;
    }
    if (length < prefix_length) {
        prefix_length = length;
    }
    if (!(flags & 0x20000000)) {
        sceSifWriteBackDCache(buffer, length);
    }
    buffer = (u8 *)((u32)buffer | 0x20000000);
    request->prefix_length = prefix_length;
    for (prefix_index = 0; prefix_index < prefix_length; prefix_index++) {
        request->prefix[prefix_index] = buffer[prefix_index];
    }
    if (sif_call_rpc(&fs_rpc_client_r2, 3, 0, &fs_rpc_send_buffer_r4, 0x30, fs_rpc_receive_buffer_r, 4, 0, 0) < 0) {
        delete_semaphore_r5(completion_semaphore);
        fs_call_sema_signal_r4();
        return -11;
    }
    rpc_result = *(s32 *)((u32)fs_rpc_receive_buffer_r | 0x20000000);
    fs_call_sema_signal_r4();
    if (rpc_result == 0) {
        delete_semaphore_r5(completion_semaphore);
        return -11;
    }
    if (flags & 0x8000) {
        delete_semaphore_r5(completion_semaphore);
        return 0;
    }
    wait_semaphore(completion_semaphore);
    delete_semaphore_r5(completion_semaphore);
    return result;
}

LINKER_REMNANT("asm/remnants/core_text", func_0011CAE0);

extern int sceSifBindRpc(void *, int, int);
extern char iop_heap_rpc_client[];
extern int iop_heap_bind_state;

/* sceSifInitIopHeap: bind the RPC client iop_heap_rpc_client to IOP server
   0x80000003, busy-waiting until the server answers; -1 if binding fails.
   Built with -fno-schedule-insns (config/func_cflags.txt), the flag
/* sce_sif_init_iop_heap: matched only with -fno-schedule-insns, which the rest of its object does not build with
   (docs/reference/BUILD_FIDELITY.md, "Removed"). */
int sce_sif_init_iop_heap(void) {
    int k;
    int r;

    for (;;) {
        r = sceSifBindRpc(iop_heap_rpc_client, 0x80000003, 0);
        if (r < 0) {
            return -1;
        }
        if (*(int *)(iop_heap_rpc_client + 0x24) != 0) {
            iop_heap_bind_state = 0;
            break;
        }
        for (k = 0x100000; k != -1; k--) {
        }
    }
    return 0;
}

extern int iop_heap_rpc_reply;
extern int iop_heap_rpc_arg;

/* Another sif_call_rpc RPC (see sce_dbc_get_mod_version), guarded on the handle
   iop_heap_bind_state being valid. Both failure exits share the single `return 0`
   that the early guard branches to. */
int sce_sif_alloc_iop_heap(int arg0) {
    if (iop_heap_bind_state < 0) {
        return 0;
    }
    iop_heap_rpc_arg = arg0;
    if (sif_call_rpc(iop_heap_rpc_client, 1, 0, &iop_heap_rpc_arg, 4, &iop_heap_rpc_reply, 4, 0, 0) >= 0) {
        return iop_heap_rpc_reply;
    } else {
        return 0;
    }
}

/* Sibling of sce_sif_alloc_iop_heap with command id 2 and -1 as the RPC failure
   result, so the two failure exits cannot share a return.

   Defined old-style ON PURPOSE. sce_sif_free_iop_heap just below calls this with
   NO argument at all -- retail's call has a bare nop in its delay slot --
   which a prototype would reject. A K&R definition creates no prototype,
   so both functions compile as retail's source evidently did. */
/* clang-format off */
int func_0011CC38(arg0)
int arg0;
{
    /* clang-format on */
    if (iop_heap_bind_state < 0) {
        return 0;
    }
    iop_heap_rpc_arg = arg0;
    if (sif_call_rpc(iop_heap_rpc_client, 2, 0, &iop_heap_rpc_arg, 4, &iop_heap_rpc_reply, 4, 0, 0) >= 0) {
        return iop_heap_rpc_reply;
    } else {
        return -1;
    }
}

/* Returns its callee's value: retail keeps the frame and calls, and
   2.9-ee would tail-call it (a bare `j`) if it were void. */
int sce_sif_free_iop_heap(void) {
    return free_sif_system_memory();
}

LINKER_REMNANT("asm/remnants/core_text", func_0011CCD0);
