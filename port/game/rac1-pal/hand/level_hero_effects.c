/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Native-only reconstruction of PAL 00205FF0..002064C0. Model color pulse,
 * flash envelope and four temporary joint manipulators, decoded from retail
 * assembly. This is not a byte-matched PS2 decompilation.
 */
extern unsigned char hero_fx_state[] __asm__("D_0013E633");
extern unsigned char hero_fx_mode[] __asm__("D_0015EEB0");
extern unsigned char hero_fx_settings[] __asm__("D_0015EEF0");
extern int hero_fx_frame __asm__("D_L00_0015F6B0");
/* gp 00166D00 - 7528 */
extern int hero_fx_joints[] __asm__("D_L00_0015F7D8");
extern unsigned int hero_fx_template_a[] __asm__("D_L00_0017C2E0");
extern unsigned int hero_fx_template_b[] __asm__("D_L00_0017C320");
extern unsigned int hero_fx_template_c[] __asm__("D_L00_0017C360");
extern float hero_fx_envelope[] __asm__("D_L00_0017C3A0");
extern float hero_fx_approach(float*, float, float) __asm__("func_00214D28");
extern int hero_fx_ticks(int) __asm__("func_001F9850");
extern float hero_fx_float(int) __asm__("func_001FA888");
extern int hero_fx_truncate(float) __asm__("func_001FA898");
extern float hero_fx_sin(float) __asm__("func_001F9FA8");
extern int hero_fx_countdown(short*) __asm__("func_001F9938");
extern int hero_fx_random(int, int) __asm__("func_L00_00258BC8");
extern void hero_fx_attach(void*, int, void*) __asm__("func_0020D960");
extern void hero_fx_detach(void*, void*) __asm__("func_0020D9D8");
extern void hero_fx_scale(void*, void*, float) __asm__("func_001F9C30");
extern void hero_fx_trap(void) __asm__("func_001F9978");

void func_L00_00205FF0(void) {
    unsigned char* g = hero_fx_state + 0xE1D;
    unsigned char *moby, *node;
    int period, red, green, blue, red_pulse, green_pulse, blue_pulse;
    int rise, fall, duration, timer, low, high, i, j;
    float phase, wave, strength;

    hero_fx_approach((float*)(hero_fx_settings + 0x28), hero_fx_mode[3] ? 1.8f : 1.0f, 0.05f);
    if (g[0x20A4] == 1 || g[0x20A4] == 2) {
        moby = *(unsigned char**)(g + 0x2080);
    } else {
        if (*(short*)(g + 0x22D8) != 0) {
            return;
        }
        moby = *(unsigned char**)(g + 0x1184);
    }
    if (!moby) {
        return;
    }
    *(unsigned short*)(moby + 0x34) |= 0x10;
    red = *(int*)(g + 0x22A8) == 1 ? 136 : 56;
    period = hero_fx_ticks(110);
    if (period == 0) {
        /* Retail raises break 7 here; keep invalid timing fatal on the host. */
        hero_fx_trap();
        return;
    }
    phase = (float)(hero_fx_frame % period) / hero_fx_float(period);
    phase += phase;
    phase *= 3.141592741f;
    wave = hero_fx_sin(phase - 3.141592741f);
    red_pulse = hero_fx_truncate(wave * 24.0f) + 12;
    green_pulse = hero_fx_truncate(wave * 48.0f) + 24;
    blue_pulse = hero_fx_truncate(wave * 24.0f) + 12;
    red += red_pulse;
    green = green_pulse + 136;
    blue = blue_pulse + 64;
    timer = *(short*)(g + 0x1EE);
    if (timer != 0) {
        strength = 1.0f;
        rise = hero_fx_ticks(5);
        fall = hero_fx_ticks(20);
        duration = hero_fx_ticks(45);
        if (duration - rise < timer) {
            duration = hero_fx_ticks(45);
            strength = hero_fx_float(duration - timer) / hero_fx_float(rise);
        }
        if (timer < fall) {
            strength = hero_fx_float(timer) / hero_fx_float(fall);
        }
        red -= hero_fx_truncate((float)red_pulse * strength);
        green -= hero_fx_truncate((float)green_pulse * strength);
        blue -= hero_fx_truncate((float)blue_pulse * strength);
        red += hero_fx_truncate(112.0f * strength);
        green -= hero_fx_truncate(72.0f * strength);
    }
    /* Retail ORs the whole channels, without byte masks or saturation. */
    *(unsigned int*)(moby + 0x90) =
        0x80000000u | ((unsigned int)blue << 16) | ((unsigned int)green << 8) | (unsigned int)red;
    if (g[0x20A4] != 0) {
        return;
    }
    if (hero_fx_countdown((short*)(g + 0xFFC)) != 0 && moby[0x53] == 1) {
        low = hero_fx_ticks(50);
        high = hero_fx_ticks(200);
        timer = hero_fx_random(low, high);
        *(short*)(g + 0xFFE) = 1;
        *(short*)(g + 0xFFC) = (short)timer;
    }
    if (*(short*)(g + 0xFFE) == 0) {
        return;
    }
    *(short*)(g + 0xFFE) = (short)(*(unsigned short*)(g + 0xFFE) + 1);
    if (*(short*)(g + 0xFFE) >= 22) {
        *(short*)(g + 0xFFE) = 0;
    }
    if (*(short*)(g + 0xFFE) == 0) {
        for (i = 0; i < 4; ++i) {
            node = g + 0xEF0 + i * 0x40;
            if (node[1] != 0) {
                hero_fx_detach(moby, node);
            }
        }
        return;
    }
    for (i = 0; i < 4; ++i) {
        node = g + 0xEF0 + i * 0x40;
        if (node[1] == 0) {
            hero_fx_attach(moby, hero_fx_joints[i], node);
            node[3] = 1;
            for (j = 0; j < 4; ++j) {
                ((unsigned int*)(node + 0x10))[j] = hero_fx_template_a[i * 4 + j];
                ((unsigned int*)(node + 0x20))[j] = hero_fx_template_b[i * 4 + j];
                ((unsigned int*)(node + 0x30))[j] = hero_fx_template_c[i * 4 + j];
            }
            hero_fx_scale(node + 0x30, node + 0x30, *(float*)(moby + 0x2C));
        }
    }
    for (i = 0; i < 4; ++i) {
        *(float*)(g + 0xEFC + i * 0x40) = hero_fx_envelope[*(short*)(g + 0xFFE)];
    }
}
