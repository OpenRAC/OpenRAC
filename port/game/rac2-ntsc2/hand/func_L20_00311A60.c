/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/* Yeedil's level script (level 20, 0x00311A60): rac2-decomp's USA v2.00 C
 * (src/us2/levels/20_yeedil/L20_00311A60-g8.c) in the release's names, with one change. Its C
 * calls the smoke puff helper func_L03_003277D0 with no argument, but the retail code passes
 * the object in $a0 (lw $a0 from the object list, then jal), as v1.01 and PAL's C do. With no
 * argument the port handed the helper a stray pointer and crashed in the boss fight.
 *
 * hostgen puts this after the matched file's C (prep.use_overrides), so its types, declarations
 * and helpers are the file's own. */

extern void func_L03_003277D0(void *object);

void func_L20_00311A60(void *incoming)
{
    int lower, upper;
    void *target;
    /* 29FC20 uses the fourth row at output+0x30; 83678 copies only
    * the first three rows. The same workspace later holds one vector. */
    Vector matrix[4], work, offset;
    float value;

    func_L00_002FD5B0(incoming);
    {
        int i;
        for (i = 0; i < D_L20_001BDC40.count; ++i) {
            u8 *object;
            object = D_L20_001BDC40.refs.objects[i];
            if (object && *(s16 *)(object+0xAA) == 0xE29)
                *(u16 *)(object+0x34) |= 0x800;
            if (*(s16 *)(object+0xAA) == 0xD54)
                FUN_002E72D8(object);
            if (object) {
                /* Set ahead of the load: its longer life is what lets loop.c move
                 * this constant out in its first pass, ahead of the object pointer. */
                int wanted = MOBY_CLASS_TV_SCREENS;
                if (*(s16 *)(object+0xAA) == wanted && D_001A79F8 != 0x33 && *(void **)(object+0x68))
                    func_L00_003272C0((char *)object,(float *)(*(u8 **)(object+0x68)+0xC),
                                      0x80AAAAD2,0x80396464,0.06108653545f);
            }

        }
    }
    if (D_001A79F8 != 20) return;
    {
        if ((u32)(D_L20_001BDC40.mode-4) < 2) {
            {
                YeedilHeroPrefix *hero = &D_00189E20;
                int owner = D_L20_001BDC40.refs.named.owner;
                hero->owner = owner;
                if (D_L20_001BDC40.mode == 4) {
                    hero->owner = D_L20_001BDC40.refs.named.secondary_owner;
                    target = D_L20_001BDC40.refs.named.secondary_target;
                    lower = D_L20_001A9434;
                    upper = D_L20_001A9438;
                } else {
                    lower = D_L20_001A943C;
                    /* Same value as the store above. It leaves no instruction of its
                     * own, but it keeps owner live into this arm. */
                    hero->owner = owner;
                    target = D_L20_001BDC40.refs.named.alternate;
                    upper = D_L20_001A9440;
                }
                {
                    YeedilCameraPrefix *camera = &D_L20_001C6A60;
                    if (D_L20_001BDC40.state == lower) {
                        func_00283410(camera,0,0x50);
                        D_L20_001AF370[0] = 1;
                        camera->scale = 0.8f;
                    }
                    if (lower < D_L20_001BDC40.state && D_L20_001BDC40.state < upper) {
                        func_002A0A58(target,0,matrix);
                        func_00284028(D_0018A550,matrix);
                        value = camera->scale;
                        value += (12.0f-value)*0.07f;
                        camera->scale = value;
                        func_002A0AF8(target,0,camera);
                        func_002838D0(&camera->point,D_0018A550,camera->scale);
                        func_00283670(&camera->point,camera,&camera->point);
                        func_0030D8C8(target);
                        func_0030DAC8(target);
                        func_L00_002E7920(func_0030E228,target);
                        if (D_L20_001BDC40.mode == 5 && D_L20_001BDC40.state == upper-1) {
                            offset.q = 0;
                            offset.f[0] = 18.0f; offset.f[1] = -2.5f; offset.f[2] = 1.0f;
                            func_00283A48(&offset,&offset,D_L20_001BA170);
                            func_00283670(&offset,&offset,D_L20_001BA170-0x230);
                            func_002836A0(&work,D_L20_001BA170-0x230,&offset);
                            func_002838D0(&work,&work,1.0f);
                            func_0032E878(*(void **)(D_0018A550-0x170),&offset,&work,0.0f);
                        }
                    } else D_00189E20.owner = 0;
                }
            }
        }
    }
    {
        int i;
        u8 *object;
        if ((u32)(D_L20_001BDC40.mode-2) < 3) {
            for (i = 0; i < D_L20_001BDC40.count; ++i) {
                u16 word;
                object = D_L20_001BDC40.refs.objects[i];
                word = *(u16 *)(object+0xAA);
                if ((u32)(word-MOBY_CLASS_MEGACORP_TROOPER_TORSO) < 2) {
                    *(u16 *)(object+0x7C) = word = (s16)word == MOBY_CLASS_MEGACORP_TROOPER_LEGS ? 15 : 0x3FF;
                }
                if (*(s16 *)(object+0xAA) == 0xCE0 && (D_L20_001BDC40.mode != 3 || D_L20_001BDC40.state < D_L20_001A9430))
                    func_L03_003277D0(object);
            }
        }
        if (D_L20_001BDC40.mode == 3 && D_L20_001BDC40.state == D_L20_001A9430) {
            qcopy(matrix,(u8 *)D_L20_001BDC40.refs.named.target+0x10);
            func_00283670(matrix,matrix,&D_L20_001A9420);
            func_0032D520(matrix,0,0x19,0x14,0x14,0x14,0x2D,1,1.0f,20.0f,1,1,0,0);
            {
                YeedilEffectPrefix *effect = &D_L20_001B9E00;
                effect->scale1 = 0.09f;
                effect->count1 = 0x37;
                effect->scale2 = 0.08f;
                effect->count2 = 0x46;
                effect->state1 = 0;
            }
        }
    }
}

