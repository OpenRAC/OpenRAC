/* func_L15_002BEFF8 -- src/overlays/shared/vendor_00298BB8.c (functional C for the port, not a match)
 * Update of a sliding door or platform (classes 196 and 197 on level 15, 1958 on level 17): it slides
 * along its yaw (+0x48) from its home position (pvars +0x00) by a signed distance (pvars +0x14),
 * opening when a linked moby (pvars +0x10, a moby index, -1 for none) reaches state 4 or when a
 * trigger (pvars +0x18) fires, and closing when the other trigger (pvars +0x1C) clears. Classes
 * 0xC4 and 0x7A6 slide the other way and play their class sound when they start moving. A door
 * with no link and an unset trigger deletes itself. States: 0 home, 1 wait for the link, 2 open,
 * 3 open for good, 4 wait for the open trigger, 5 wait for the close trigger, 6 close.
 * equiv: NEAR (all copies: retail rebuilds the 3.0 constant after the calls, this keeps it). */
typedef unsigned int Q_2BEFF8 __attribute__((mode(TI), aligned(16)));
typedef struct {
    Q_2BEFF8 home;
    int link;
    float dist;
    int openTrig;
    int closeTrig;
} Pv_2BEFF8;
typedef struct {
    unsigned char pad0[0x24];
    float len;
} Cls_2BEFF8;
typedef struct {
    unsigned char pad0[0x10];
    Q_2BEFF8 pos;
    unsigned char state;
    unsigned char pad21[3];
    Cls_2BEFF8 *cls;
    unsigned char pad28[4];
    float scale;
    unsigned char pad30[0x48 - 0x30];
    float yaw;
    unsigned char pad4C[0x78 - 0x4C];
    Pv_2BEFF8 *pv;
    unsigned char pad7C[0xA6 - 0x7C];
    short oclass;
} Moby_2BEFF8;

extern int D_L15_00160058_2BEFF8 __asm__("D_L15_00160058") MACRO_ADDR;
extern float D_0015EE6C_2BEFF8 __asm__("D_0015EE6C") MACRO_ADDR;
extern char D_0013F4D0_2BEFF8[] __asm__("D_0013F4D0");

extern void func_0020D678_2BEFF8(void *) __asm__("func_0020D678");
extern void func_0022ED80_2BEFF8(int, int, void *) __asm__("func_0022ED80");
extern float func_001F9F90_2BEFF8(float) __asm__("func_001F9F90");
extern float func_001F9FA8_2BEFF8(float) __asm__("func_001F9FA8");
extern void func_001F9BD8_2BEFF8(void *, void *, void *) __asm__("func_001F9BD8");
extern float func_001F9B88_2BEFF8(float) __asm__("func_001F9B88");
extern int func_00215570_2BEFF8(void *, int) __asm__("func_00215570");

void func_L15_002BEFF8(Moby_2BEFF8 *m) {
    Pv_2BEFF8 *pv = m->pv;
    int rev = 0;
    float v[4];
    Q_2BEFF8 t;

    if (m->oclass == 0xC4 || m->oclass == 0x7A6) {
        rev = 1;
    }
    if (pv->link == -1 && (pv->openTrig == -1 || pv->closeTrig == -1)) {
        func_0020D678_2BEFF8(m);
        return;
    }
    switch (m->state) {
    case 0:
        pv->home = m->pos;
        if (pv->link != 0) {
            m->state = 1;
        } else {
            m->state = 4;
        }
        break;
    case 1:
        if (((unsigned char *)((pv->link << 8) + D_L15_00160058_2BEFF8))[0x20] != 4) {
            return;
        }
        if (rev) {
            func_0022ED80_2BEFF8(0, 0, m);
        }
        m->state = 2;
        break;
    case 2: {
        float a2;
        float d2 = m->scale * 3.0f / m->cls->len / 3.0f * D_0015EE6C_2BEFF8;
        if (rev) {
            d2 = -d2;
        }
        pv->dist = pv->dist + d2;
        v[0] = func_001F9F90_2BEFF8(m->yaw) * pv->dist;
        v[1] = func_001F9FA8_2BEFF8(m->yaw) * pv->dist;
        *(int *)&v[2] = 0;
        func_001F9BD8_2BEFF8(&t, pv, v);
        m->pos = t;
        a2 = func_001F9B88_2BEFF8(pv->dist);
        if (m->scale * 3.0f / m->cls->len < a2) {
            if (pv->link != -1) {
                m->state = 3;
            } else {
                m->state = 5;
            }
        }
        break;
    }
    case 3:
        break;
    case 4:
        pv->dist = 0.0f;
        if (func_00215570_2BEFF8(D_0013F4D0_2BEFF8, pv->openTrig) == 0) {
            return;
        }
        if (rev) {
            func_0022ED80_2BEFF8(0, 0, m);
        }
        m->state = 2;
        break;
    case 5:
        if (func_00215570_2BEFF8(D_0013F4D0_2BEFF8, pv->closeTrig) != 0) {
            return;
        }
        if (rev) {
            func_0022ED80_2BEFF8(0, 0, m);
        }
        m->state = 6;
        break;
    case 6: {
        float a6 = func_001F9B88_2BEFF8(pv->dist);
        float d6 = m->scale * 3.0f / m->cls->len / 3.0f * D_0015EE6C_2BEFF8;
        float n6 = -d6;
        if (d6 < a6) {
            float r6;
            if (rev) {
                r6 = pv->dist - n6;
            } else {
                r6 = pv->dist - d6;
            }
            pv->dist = r6;
        } else {
            pv->dist = 0.0f;
        }
    }
        v[0] = func_001F9F90_2BEFF8(m->yaw) * pv->dist;
        v[1] = func_001F9FA8_2BEFF8(m->yaw) * pv->dist;
        *(int *)&v[2] = 0;
        func_001F9BD8_2BEFF8(&t, pv, v);
        m->pos = t;
        if (pv->dist == 0.0f) {
            m->state = 4;
        }
        break;
    }
}
