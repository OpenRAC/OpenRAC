/* GPT-6.1 Sol high, 29 October 2026. Game -O2 -G0 -ffunction-sections.
 * Best assigned-unit check: 912/912 bytes, 0/28 USA v1.01 places; first
 * non-relocation difference +0x160. Sixteen exploratory checker invocations this task.
 * Corrected the older draft: a true func_002F7320 result MUST set the bit;
 * retail branches from 0x002F744C directly to the shift at 0x002F74D0.
 * Separate key/channel/value lifetimes for the two scans, signed guard offsets,
 * a named count word, macro-address flag arrays and a separate stored-value
 * local reproduce most instructions. The second-scan preheader and address
 * setup at 0x002F74FC..0x002F7584 still differ (including high-half registers).
 * A separate mapping local regresses to 940 bytes; changing the active-class
 * pointer to a byte pointer ties. Required declarations are below; common.h
 * supplies MACRO_ADDR. F73State is a layout view of the existing state symbol.
 * No linked build run. Next: second-scan base/declaration lifetime evidence.
 */
#include "common.h"

extern unsigned char D_00139568[];
extern unsigned char D_001A7AB8[] MACRO_ADDR;
extern unsigned char D_001A7AF8[] MACRO_ADDR;
extern unsigned char D_001A7A80[] MACRO_ADDR;
typedef struct { int active; char pad4[0x10]; int info; char pad18[0x80]; int channels[3]; char padA4[0x3C]; } F73Record;
extern F73Record D_00239BA0[];
typedef struct { char pad[0x140]; int rows[96][4]; int count; } F73StateType;
extern F73StateType F73State __asm__("D_0021F240");
extern int func_002F7320(int owner, int key, int channel);
extern int func_00288D58(void);
extern int D_L00_0022FF00[];

extern int D_001A79F0;

void func_002F73A8(void)
{
    D_L00_0022FF00[0] = 0;
    {
        int key, channel, available;
        for (key = 0; key < 56; key++) {
            int offset = D_00139568[key] * 224;
            if (((F73Record *)((char *)D_00239BA0 + offset))->active != 0) {
                for (channel = 0; channel < 3; channel++) {
                    if (func_002F7320(D_001A79F0, key, channel) != 0)
                        goto mark_available;
                    switch (channel) {
                    case 0: available = ((F73Record *)((char *)D_00239BA0 + D_00139568[key] * 224))->channels[0]; break;
                    case 1: available = ((F73Record *)((char *)D_00239BA0 + D_00139568[key] * 224))->channels[1]; break;
                    case 2: available = ((F73Record *)((char *)D_00239BA0 + D_00139568[key] * 224))->channels[2]; break;
                    default: available = 0; break;
                    }
                    if (!available) continue;
                    if (!func_00288D58()) continue;
mark_available:
                    D_001A7AB8[key] |= 1 << channel;
                }
            }
        }
    }
    {
        int key, channel, available;
        char *entry;
        F73Record *active_classes = D_00239BA0;
        for (key = 0; key < 56; key++) {
            char *classes;
            if (key == 0) continue;
            if (active_classes[D_00139568[key]].active == 0) continue;
            if (D_001A7AF8[key] == 0) continue;
            classes = (char *)D_00239BA0;
            for (channel = 0; channel < 3; channel++) {
                {
                    unsigned int mask = 1U << channel;
                    if (!(D_001A7AB8[key] & mask)) continue;
                }
                switch (channel) {
                case 0: available = ((F73Record *)(classes + D_00139568[key] * 224))->channels[0]; break;
                case 1: available = ((F73Record *)(classes + D_00139568[key] * 224))->channels[1]; break;
                case 2: available = ((F73Record *)(classes + D_00139568[key] * 224))->channels[2]; break;
                default: available = 0; break;
                }
                if (!available) continue;
                {
                    unsigned char mask = 1 << (channel * 2);
                    if (D_001A7A80[key] & mask) continue;
                }
                F73State.rows[F73State.count][0] = channel;
                {
                    int stored_value;
                    switch (channel) {
                    case 0: stored_value = ((F73Record *)(classes + D_00139568[key] * 224))->channels[0]; break;
                    case 1: stored_value = ((F73Record *)(classes + D_00139568[key] * 224))->channels[1]; break;
                    case 2: stored_value = ((F73Record *)(classes + D_00139568[key] * 224))->channels[2]; break;
                    default: stored_value = 0; break;
                    }
                    F73State.rows[F73State.count][2] = stored_value;
                }
                entry = classes + D_00139568[key] * 224;
                F73State.rows[F73State.count][1] = key;
                F73State.rows[F73State.count][3] = *(int *)(entry + 0x14);
                ++F73State.count;
            }
        }
    }
}
