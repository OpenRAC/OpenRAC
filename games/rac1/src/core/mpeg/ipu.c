#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "core/kernel/kprintf.h"
#include "asm_functions/executable.h"

/*
 * core_text object 0x12CC90-0x12D868. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Sony SDK code, built with Sony's 2.9-ee (Makefile.sn, EE29_CORE). Up to
 * 0x12D068: IPU DMA helpers for libmpeg (the CHCR writes of channels 3/4
 * under a D_ENABLE hold). From 0x12D2A0: libscf -- the rom0:ROMVER record,
 * sceScfGetTimeZone (sce_scf_get_time_zone), sceScfGetSummerTime (sce_scf_get_summer_time),
 * the BCD<->binary sceCdCLOCK helpers and sceScfGetLocalTimefromRTC
 * (sce_scf_get_local_time_from_rtc). Its day/hour helpers end in conditional tail calls
 * (scf_add_hour, scf_sub_hour), which only 2.9-ee emits.
 */

extern void scePrintf();

extern void sce_ipu_restart_dma(unsigned int *);

/*
 * Write a CHCR value to IPU DMA channel 3 (fromIPU, 0x1000B000) or 4
 * (toIPU, 0x1000B400): with interrupts off (disable_interrupts/enable_interrupts),
 * set D_ENABLEW.CPND (0x1000F590 = D_ENABLER | 0x10000) to hold the DMAC,
 * store the value, release the hold. The final EI is a tail call.
 *
 * Exact under 2.9-ee as the plain volatile MMIO sequence. It was blocked
 * under 2.95.3 only because that compiler never emits a sibling call:
 * with a frame of its own the last call stayed a `jal` plus a full
 * epilogue, 3 words over (0x70 against 0x64).
 */
void set_d3_chcr(unsigned int chcr) {
    disable_interrupts();
    *(volatile unsigned int *)0x1000F590 = *(volatile unsigned int *)0x1000F520 | 0x10000;
    *(volatile unsigned int *)0x1000B000 = chcr;
    *(volatile unsigned int *)0x1000F590 = *(volatile unsigned int *)0x1000F520 & 0xFFFEFFFF;
    enable_interrupts();
}

void set_d4_chcr(unsigned int chcr) {
    disable_interrupts();
    *(volatile unsigned int *)0x1000F590 = *(volatile unsigned int *)0x1000F520 | 0x10000;
    *(volatile unsigned int *)0x1000B400 = chcr;
    *(volatile unsigned int *)0x1000F590 = *(volatile unsigned int *)0x1000F520 & 0xFFFEFFFF;
    enable_interrupts();
}

/* sceIpuStopDMA: request the IPU-in (D4/toIPU) DMA channel to stop
 * (set_d4_chcr(1)), save its MADR/TADR/QWC/CHCR into env[0..3], wait for
 * the IPU's queued-command count (IPU_CTRL bits 4-7) to drain, stop the
 * IPU-out (D3/fromIPU) channel (set_d3_chcr(0)) and save its
 * MADR/QWC/CHCR plus IPU_BP and IPU_CTRL into env[4..8]. */
extern void set_d4_chcr(unsigned int);
extern void set_d3_chcr(unsigned int);

void sce_ipu_stop_dma(unsigned int *env) {
    set_d4_chcr(1);
    env[0] = *(volatile unsigned int *)0x1000B410;
    env[1] = *(volatile unsigned int *)0x1000B430;
    env[2] = *(volatile unsigned int *)0x1000B420;
    env[3] = *(volatile unsigned int *)0x1000B400;
    while ((*(volatile unsigned int *)0x10002010 & 0xF0) != 0) {
    }
    set_d3_chcr(0);
    env[4] = *(volatile unsigned int *)0x1000B010;
    env[5] = *(volatile unsigned int *)0x1000B020;
    env[6] = *(volatile unsigned int *)0x1000B000;
    env[7] = *(volatile unsigned int *)0x10002020;
    env[8] = *(volatile unsigned int *)0x10002010;
}

/* sceIpuRestartDMA: given the env sceIpuStopDMA (sce_ipu_stop_dma) filled,
 * work out how many quadwords the IPU's bitstream pointer (env[7], IPU_BP)
 * had already consumed (its FIFO+chain counts, bits 8-10/16-17) and fold
 * that back into the saved D4 MADR/QWC; restart D3 (fromIPU) first if it
 * still had a QWC left, reissue the BP position to the IPU command
 * register, wait for the IPU to go idle, then restart D4 (toIPU) with the
 * corrected MADR/QWC if there is anything left to transfer.
 *
 * The masked BP value needs its own local right after the load: masking
 * `bp` again at the store keeps it alive across set_d3_chcr and moves
 * qwc4 and madr4 into the wrong saved registers. */

void sce_ipu_restart_dma(unsigned int *env) {
    unsigned int bp = env[7];
    unsigned int bpLow = bp & 0x7F;
    unsigned int madrSaved = env[0];
    unsigned int cnt = ((bp >> 16) & 3) + ((bp >> 8) & 0xF);
    unsigned int d3madr = env[4];
    unsigned int qwc4 = env[2] + cnt;
    unsigned int madr4 = madrSaved - (cnt << 4);

    if (d3madr != 0 && env[5] != 0) {
        *(volatile unsigned int *)0x1000B010 = d3madr;
        *(volatile unsigned int *)0x1000B020 = env[5];
        set_d3_chcr(env[6] | 0x100);
    }

    while (*(volatile int *)0x10002010 < 0) {
    }
    *(volatile unsigned int *)0x10002000 = bpLow;
    while (*(volatile int *)0x10002010 < 0) {
    }

    if (madr4 == 0) {
        return;
    }
    if (qwc4 == 0) {
        return;
    }
    *(volatile unsigned int *)0x1000B410 = madr4;
    *(volatile unsigned int *)0x1000B430 = env[1];
    *(volatile unsigned int *)0x1000B420 = qwc4;
    set_d4_chcr(env[3] | 0x100);
}

int sce_ipu_sync(int arg0) {
    int ret = 0;

    switch (arg0) {
        case 0:
            while (*(volatile int *)0x10002010 < 0) {
            }
            ret = 0;
            break;
        case 1:
            ret = *(volatile unsigned int *)0x10002010 >> 31;
            break;
    }

    return ret;
}

/* Sony's setD4_CHCR, linked a second time as ipuinit.o's own private copy
 * (called from sceIpuInit, which immediately follows at 0x12D068): write
 * chcr to IPU DMA channel 4's CHCR (toIPU, 0x1000B400) with interrupts off
 * and D_ENABLEW.CPND (0x1000F590 = D_ENABLER | 0x10000) held around it.
 * Byte-identical to set_d4_chcr (setD4_CHCR's other copy, already
 * matched) since it is the same source compiled into a different object.
 */
void set_d4_chcr_ipuinit(unsigned int chcr) {
    disable_interrupts();
    *(volatile unsigned int *)0x1000F590 = *(volatile unsigned int *)0x1000F520 | 0x10000;
    *(volatile unsigned int *)0x1000B400 = chcr;
    *(volatile unsigned int *)0x1000F590 = *(volatile unsigned int *)0x1000F520 & 0xFFFEFFFF;
    enable_interrupts();
}

typedef unsigned int u128 __attribute__((mode(TI), aligned(16)));

extern void set_d4_chcr_ipuinit(unsigned int chcr);
extern volatile u128 ipu_init_quant_matrices[];
extern u128 ipu_init_vq_clut[];

#define IPU_CMD ((volatile unsigned int *)0x10002000)
#define IPU_CTRL ((volatile int *)0x10002010)
#define IPU_IN_FIFO ((volatile u128 *)0x10007010)

/* sceIpuInit: reset the IPU, load its two fixed FIFO command tables, reset again. Adapted from Lombyte's project (GPL-3.0) for PAL: src/sdk/dma/sce_ipu_init.c, sceIpuInit. */
void sce_ipu_init(void) {
    set_d4_chcr_ipuinit(1);
    *IPU_CTRL = 0x40000000;
    while (*IPU_CTRL < 0) {
    }
    *IPU_CMD = 0;
    while (*IPU_CTRL < 0) {
    }
    *IPU_IN_FIFO = ipu_init_quant_matrices[0];
    *IPU_IN_FIFO = ipu_init_quant_matrices[1];
    *IPU_IN_FIFO = ipu_init_quant_matrices[2];
    *IPU_IN_FIFO = ipu_init_quant_matrices[3];
    *IPU_IN_FIFO = ipu_init_quant_matrices[4];
    *IPU_IN_FIFO = ipu_init_quant_matrices[4];
    *IPU_IN_FIFO = ipu_init_quant_matrices[4];
    *IPU_IN_FIFO = ipu_init_quant_matrices[4];
    *IPU_CMD = 0x50000000;
    while (*IPU_CTRL < 0) {
    }
    *IPU_CMD = 0x58000000;
    while (*IPU_CTRL < 0) {
    }
    *IPU_IN_FIFO = ipu_init_vq_clut[0];
    *IPU_IN_FIFO = ipu_init_vq_clut[1];
    *IPU_CMD = 0x60000000;
    while (*IPU_CTRL < 0) {
    }
    *IPU_CMD = 0x90000000;
    while (*IPU_CTRL < 0) {
    }
    *IPU_CTRL = 0x40000000;
    while (*IPU_CTRL < 0) {
    }
    *IPU_CMD = 0;
    while (*IPU_CTRL < 0) {
    }
}

/*
 * Fill the 14-byte rom0:ROMVER record at RomNameStateData once (open
 * scf_romver_path "rom0:ROMVER" read-only, read 0xE bytes, close; failures are
 * reported through scePrintf) and hand the buffer back either way.
 * (The second scePrintf gets only the format string: $a1 still holds
 * the buffer from the read call and retail never resets it, which is why
 * scePrintf stays unprototyped here.)
 *
 * Exact under 2.9-ee. It was blocked under 2.95.3 (152/160 against 156):
 * that compiler's GCSE copies the kept %hi(RomNameStateData) into a saved
 * register (`move $19,$2`), where 2.9-ee loads the lui straight into $17
 * as retail does. RomNameStateData is `signed char` (the declaration below),
 * hence the cast on the return.
 */
extern int sceOpen(void *, int);

extern void sceClose(int);
extern signed char RomNameStateData[];
extern char scf_romver_path[], scf_romver_open_failed_msg[], scf_romver_read_failed_msg[];

char *scf_read_romver(void) {
    int fd;
    if (RomNameStateData[0] == 0) {
        fd = sceOpen(scf_romver_path, 1);
        if (fd == -1) {
            scePrintf(scf_romver_open_failed_msg, RomNameStateData);
        }
        if (sceRead(fd, RomNameStateData, 0xE) == -1) {
            scePrintf(scf_romver_read_failed_msg);
        }
        sceClose(fd);
    }
    return (char *)RomNameStateData;
}

extern char *scf_read_romver(void);

int scf_is_t10k(void) {
    if (RomNameStateData[0] == 0) {
        scf_read_romver();
    }
    return RomNameStateData[4] == 0x54;
}

/*
 * Both of these return a flag byte that retail loads with the address
 * register reused as the destination (`lui $2,%hi(X)` / `lbu
 * $2,%lo(X)($2)`), and it loads it ACROSS TWO DELAY SLOTS -- the lui in
 * the branch's, the lbu in the following b's. That rules MACRO_ADDR out
 * here, and the attempt is worth recording: a MACRO_ADDR access in a
 * delay slot is expanded $gp-relative, and these two symbols live
 * outside the +-32KB window, so the link fails outright with
 * "relocation truncated to fit: R_MIPS_GPREL16". A plain incomplete
 * array gives retail's exact split-across-delay-slots shape; the only
 * residual is the allocator putting the address in $3 rather than
 * reusing $2, plus one more register choice in sce_scf_get_summer_time
 * (5/96 and 6/104 differing bytes). sce_scf_get_summer_time also needed its arms
 * the other way round -- the early `return 0` written last, so the
 * zero case is the one that branches over.
 */
extern unsigned char ScfLanguage[];

/* One return, the `== 0` arm first, and `v` unsigned (as sce_scf_get_time_zone). */
int sce_scf_get_language(void) {
    int buf[4];
    unsigned int v;
    int r;

    get_osd_config_param(buf);
    if (scf_is_t10k() != 0) {
        r = ScfLanguage[0];
    } else {
        get_osd_config_param(buf);
        v = buf[0];
        if (((v >> 13) & 7) == 0) {
            r = (v >> 4) & 1;
        } else {
            r = (v >> 16) & 0x1F;
        }
    }
    return r;
}

LINKER_REMNANT("asm/remnants/core_text", func_0012D3E0);

extern short scf_t10k_time_zone NOT_SDA;

/* `r` assigned in both arms and returned once: a direct `return
   scf_t10k_time_zone;` loads straight into $v0 and moves the address to $v1. */
int sce_scf_get_time_zone(void) {
    unsigned int local;
    unsigned int bits;
    int r;

    if (scf_is_t10k() != 0) {
        r = scf_t10k_time_zone;
    } else {
        get_osd_config_param(&local);
        bits = (local >> 13) & 7;
        if (bits == 0) {
            r = 0x21C;
        } else {
            r = (int)local >> 21;
        }
    }
    return r;
}

LINKER_REMNANT("asm/remnants/core_text", func_0012D440);

extern unsigned char scf_t10k_summer_time[];

/* One return, and GetOsdConfigParam2 returns int. */
int sce_scf_get_summer_time(void) {
    unsigned char buf[16];
    int r;

    if (scf_is_t10k() != 0) {
        r = scf_t10k_summer_time[0];
    } else {
        get_osd_config_param(buf);
        if (((*(unsigned int *)buf >> 13) & 7) == 0) {
            r = 0;
        } else {
            GetOsdConfigParam2(buf + 4, 1, 1);
            r = (buf[4] >> 4) & 1;
        }
    }
    return r;
}

int scf_bin_to_bcd(int arg0) {
    unsigned int v = arg0 & 0xFF;
    return ((v / 10) * 6 + v) & 0xFF;
}

/* BCD byte to binary. The product goes through an unsigned char: combine
   folds the multiply into that narrowing copy and re-recognises it as the
   generic LO-destination multiply, which is retail's `mult $0,a,b` +
   `mflo` (the plain int form picks the R5900 three-operand mult). */
/* The product goes through an unsigned char local: that gives retail's
   generic mult $0 + mflo instead of the three-operand mult. */
int scf_bcd_to_bin(int arg0) {
    unsigned char v = arg0;
    unsigned char t = (v >> 4) * 6;
    return (unsigned char)(v - t);
}

extern int scf_bcd_to_bin(int);

/* Convert the sceCdCLOCK fields (second..year) in place with the helper
   above. Exact under 2.9-ee, which saves $16 before $31 as retail does;
   2.95.3 saved them the other way round (4/104), the prologue-order
   residual that pointed at the SDK compiler in the first place. */
void decode_bcd_time_fields(unsigned char *p) {
    p[7] = scf_bcd_to_bin(p[7]);
    p[6] = scf_bcd_to_bin(p[6]);
    p[5] = scf_bcd_to_bin(p[5]);
    p[3] = scf_bcd_to_bin(p[3]);
    p[2] = scf_bcd_to_bin(p[2]);
    p[1] = scf_bcd_to_bin(p[1]);
}

extern int scf_bin_to_bcd(int);

/* The inverse conversion, same shape; exact under 2.9-ee for the same
   reason as decode_bcd_time_fields (4/104 under 2.95.3). */
void encode_bcd_time_fields(unsigned char *p) {
    p[7] = scf_bin_to_bcd(p[7]);
    p[6] = scf_bin_to_bcd(p[6]);
    p[5] = scf_bin_to_bcd(p[5]);
    p[3] = scf_bin_to_bcd(p[3]);
    p[2] = scf_bin_to_bcd(p[2]);
    p[1] = scf_bin_to_bcd(p[1]);
}

typedef struct {
    char b[0xC];
} Cfg12;
extern Cfg12 MonthLengths NOT_SDA;

/*
 * Tick the sceCdCLOCK at s (binary fields: [5] day, [6] month, [7] year)
 * forward by one day, with a working copy of the 12-byte month-length
 * table MonthLengths (February stretched to 29 when year & 3 == 0), the
 * month rolling 12 -> 1 and the year 99 -> 0.
 *
 * The leap test reads the year into an int. Retail loads the year twice
 * (for the leap test, and again at the year roll-over); 2.9-ee's GCSE
 * would reuse a first byte-typed load for the second (SIZE 184/180, and
 * the whole reason this function once looked like 2.95.3 code). An int
 * load is a zero-extending load, a different expression to GCSE, so
 * the later byte load stays. (This C is exact under 2.95.3 as well.)
 */
void scf_add_day(unsigned char *s) {
    Cfg12 days;
    unsigned char m;
    int y = s[7];

    days = MonthLengths;
    s[5] = s[5] + 1;
    if ((y & 3) == 0) {
        days.b[1] = 0x1D;
    }
    if (s[5] > days.b[s[6] - 1]) {
        s[5] = 1;
        s[6] = s[6] + 1;
        m = s[6];
        if (m == 13) {
            if (s[7] == 0x63) {
                s[7] = 0;
            } else {
                s[7] = s[7] + 1;
            }
            s[6] = 1;
        }
    }
}

/*
 * Tick the sceCdCLOCK at s back by one day: the same month-length table,
 * the day counting down to 0 rolls the month back (1 -> 12) and the year
 * with it (0 -> 99), then the day is reloaded from the table.
 *
 * Levers: the leap test on an int copy of the year, as in scf_add_day
 * (2.9-ee's GCSE otherwise reuses the first year load: 61/164);
 * `s[5] = s[5] - 1;` before the leap block (a block boundary gives
 * retail's reload of s[5]); the month as `if (--s[6] == 0)` (stores the
 * raw decrement and tests the masked copy in another register); the
 * year as an if/else with one store per arm, which cross-jumping merges
 * into retail's single `sb`. Exact under both compilers.
 */
void scf_sub_day(unsigned char *s) {
    Cfg12 days;
    int y = s[7];

    days = MonthLengths;
    s[5] = s[5] - 1;
    if ((y & 3) == 0) {
        days.b[1] = 0x1D;
    }
    if (s[5] == 0) {
        if (--s[6] == 0) {
            if (s[7] == 0) {
                s[7] = 0x63;
            } else {
                s[7] = s[7] - 1;
            }
            s[6] = 0xC;
        }
        s[5] = days.b[s[6] - 1];
    }
}

/*
 * Advance the hour ([3]); at 24 it wraps to 0 and the day ticks forward.
 * Retail's call is a bare tail `j scf_add_day` on the wrap path only,
 * with a plain `jr $ra` on the other: a conditional sibling call, which
 * 2.9-ee emits from this plain C. (Under 2.95.3, which has no sibling
 * calls, it was 8 bytes over.)
 */
void scf_add_hour(unsigned char *s) {
    s[3] = s[3] + 1;
    if (s[3] == 0x18) {
        s[3] = 0;
        scf_add_day(s);
    }
}

/* The backward analog: the hour counts down, and at 0 it becomes 23 and
   the day ticks back, again through a conditional tail call. */
void scf_sub_hour(unsigned char *s) {
    unsigned char v = s[3];
    if (v != 0) {
        s[3] = v - 1;
        return;
    }
    s[3] = 0x17;
    scf_sub_day(s);
}

/*
 * Shift the sceCdCLOCK at s by diff minutes: BCD to binary
 * (decode_bcd_time_fields), add diff to the minutes, carry whole hours down or up
 * through the hour helpers, store the minutes, binary back to BCD
 * (encode_bcd_time_fields, a tail call). Note the upper bound is `> 60`, not
 * `>= 60`: retail's test is `slti 0x3D`.
 *
 * The `min >= 0` arm has to come first: retail lays the carry-up loop
 * inline and the carry-down loop after it.
 */
void scf_adjust_clock_minutes(unsigned char *s, int diff) {
    int min;

    decode_bcd_time_fields(s);
    min = s[2] + diff;
    if (min >= 0) {
        while (min > 60) {
            min -= 60;
            scf_add_hour(s);
        }
    } else {
        while (min < 0) {
            min += 60;
            scf_sub_hour(s);
        }
    }
    s[2] = min;
    encode_bcd_time_fields(s);
}

/*
 * sceScfGetLocalTimefromRTC: shift the RTC clock (Japan time, UTC+9) by
 * the configured time zone (sce_scf_get_time_zone, minutes) and summer time
 * (sce_scf_get_summer_time, hours), i.e. by tz - 540 + st * 60 minutes. Written in
 * that order: `tz + st * 60 - 540` associates differently (19/72).
 */
void sce_scf_get_local_time_from_rtc(unsigned char *s) {
    int tz, st;
    tz = sce_scf_get_time_zone();
    st = sce_scf_get_summer_time();
    scf_adjust_clock_minutes(s, tz - 540 + st * 60);
}

/* 8 bytes of post-endlabel nop padding in retail -- see FontPrint. */
__asm__(".section .text\n\tnop\n\tnop\n");
