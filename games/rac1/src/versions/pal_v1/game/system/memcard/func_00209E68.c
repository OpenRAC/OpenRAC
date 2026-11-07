typedef struct {
    s32 port;
    s32 slot;
    s32 type;
    s32 free;
    s32 format;
    s32 x14;
    s32 x18;
    s32 x1C;
    u8 pad_20[0x8C];
    s32 xAC;
    s32 xB0;
    s32 xB4;
} McCardV1;

typedef struct {
    McCardV1 card[1];
    s32 cmd;
    s32 result;
    s32 xC0;
    s32 cur;
    s32 xC8;
    s32 busy;
    s32 fd;
    s32 state;
    s32 sub;
    s32 xDC;
    s32 xE0;
    s32 err;
    s32 errCard;
    void *buf;
    s32 size;
} McStateV1;

extern s32 func_E1_001240C0(s32, s32, char *, s32);
extern char D_E1_0015FE58[];
extern char D_E1_0015FE60[];
extern char D_E1_001506D0[];

#define MC (*(McStateV1 *)&mc_data_0013D390)

void memcard_update_state(void) {
    char workspace[0x40];

    if (MC.busy) {
        MC.busy = sce_mc_sync(1, &MC.cmd, &MC.result) == 0;
        return;
    }
    MC.busy = 1;

    switch (MC.state) {
        case 0:
            if (MC.cur < 0) {
                MC.cur = 0;
            }
            if (sce_mc_get_info(MC.card[MC.cur].port, MC.card[MC.cur].slot, &MC.card[MC.cur].type,
                                &MC.card[MC.cur].free, &MC.card[MC.cur].format) == 0) {
                MC.state = 1;
            }
            break;

        case 1:
            if (MC.result != 0) {
                MC.card[MC.cur].x14 = -3;
                MC.card[MC.cur].x1C = MC.result;
                MC.card[MC.cur].xAC = -1;
            }
            if (++MC.cur < 1) {
                MC.state = 0;
            } else {
                MC.state = 2;
            }
            MC.busy = 0;
            break;

        case 2:
            if (MC.xDC >= 0) {
                if (MC.card[0].x1C == 0) {
                    if (MC.xE0 >= 0) {
                        MC.cur = MC.xE0;
                        MC.state = MC.xDC;
                        MC.err = 0;
                    } else {
                        MC.err = 0x271A;
                    }
                }
                MC.xDC = -1;
                MC.xE0 = -1;
            } else {
                MC.cur++;
                if (MC.cur >= 31) {
                    MC.cur = 0;
                    MC.state = 0;
                }
            }
            MC.busy = 0;
            break;

        case 3:
            if (MC.card[MC.cur].format == 0) {
                if (sce_mc_format(MC.card[MC.cur].port, MC.card[MC.cur].slot) == 0) {
                    MC.state = 4;
                }
                break;
            }
            MC.busy = 0;
            MC.state = 2;
            break;

        case 4:
            if (MC.result != 0) {
                MC.err = 1;
                MC.errCard = MC.cur;
            }
            MC.state = 0;
            MC.cur = 0;
            MC.busy = 0;
            break;

        case 5:
            if (MC.card[MC.cur].format == 0) {
                if (sce_mc_unformat(MC.card[MC.cur].port, MC.card[MC.cur].slot) == 0) {
                    MC.state = 6;
                }
                break;
            }
            MC.busy = 0;
            MC.state = 2;
            break;

        case 6:
            if (MC.result != 0) {
                MC.err = 2;
                MC.errCard = MC.cur;
            }
            MC.state = 0;
            MC.cur = 0;
            MC.busy = 0;
            break;

        case 7:
            if (func_E1_001240C0(MC.card[MC.cur].port, MC.card[MC.cur].slot, mc_data_0013D2D0, 0) == 0) {
                MC.sub = 0;
                MC.state = 8;
            }
            break;

        case 8:
            switch (MC.sub) {
                case 0:
                    if (MC.result == 0) {
                        if (MC.card[MC.cur].x14 < 0) {
                            MC.card[MC.cur].x14 = -1;
                        }
                        MC.sub = 1;
                        MC.busy = 0;
                    } else {
                        MC.card[MC.cur].x14 = -2;
                        if (MC.result == -2) {
                            MC.err = 3;
                            MC.errCard = MC.cur;
                        } else if (MC.result != -4) {
                            MC.err = 4;
                            MC.errCard = MC.cur;
                        }
                        MC.state = 0;
                        MC.cur = 0;
                        MC.busy = 0;
                    }
                    break;
                case 1:
                    sprintf(workspace, mc_data_0013D370, 0);
                    if (mc_call_001238B0(MC.card[MC.cur].port, MC.card[MC.cur].slot, workspace, 1) == 0) {
                        MC.sub++;
                    }
                    break;
                case 2:
                    if (MC.result >= 0) {
                        MC.fd = MC.result;
                        MC.sub = 3;
                    } else {
                        MC.card[MC.cur].x14 = -2;
                        MC.errCard = MC.cur;
                        if (MC.result == -7) {
                            MC.err = 0x2710;
                        } else if (MC.result == -5) {
                            MC.err = 0x2711;
                        } else if (MC.result == -4) {
                            MC.err = 0x2712;
                        } else if (MC.result == -3) {
                            MC.err = 0x2713;
                        } else if (MC.result == -2) {
                            MC.err = 3;
                        } else {
                            MC.err = 4;
                        }
                        MC.state = 0;
                        MC.cur = 0;
                    }
                    MC.busy = 0;
                    break;
                case 3:
                    MC.size = 8;
                    if (sce_mc_read(MC.fd, &MC.card[MC.cur].xB0, 8) == 0) {
                        MC.sub = 4;
                    }
                    break;
                case 4:
                    if (MC.result == MC.size) {
                        MC.card[MC.cur].xAC = 0;
                        if (MC.card[MC.cur].xB0 != mc_call_0020BAD8(mc_data_001A05C0)) {
                            MC.card[MC.cur].xAC++;
                        }
                        if (MC.card[MC.cur].xB4 != mc_call_0020BAD8(mc_data_001A08C0)) {
                            MC.card[MC.cur].xAC++;
                        }
                        MC.busy = 0;
                        MC.sub = 5;
                    } else {
                        MC.card[MC.cur].x14 = -2;
                        MC.errCard = MC.cur;
                        if (MC.result >= 0) {
                            MC.err = 0x2716;
                            if (sce_mc_close(MC.fd) == 0) {
                                MC.state = 0;
                                MC.cur = 0;
                                MC.busy = 0;
                            }
                            break;
                        }
                        if (MC.result == -5) {
                            MC.err = 0x2711;
                        } else if (MC.result == -4) {
                            MC.err = 0x2714;
                        } else if (MC.result == -3) {
                            MC.err = 0x2715;
                        } else if (MC.result == -2) {
                            MC.err = 3;
                        } else {
                            MC.err = 4;
                        }
                        MC.state = 0;
                        MC.cur = 0;
                        MC.busy = 0;
                    }
                    break;
                case 5:
                    if (sce_mc_close(MC.fd) == 0) {
                        MC.busy = 0;
                        MC.state = 21;
                    }
                    break;
            }
            break;

        case 9:
            MC.state = 10;
            MC.sub = 0;
            MC.card[MC.cur].x14 = 0;
            /* fall through */
        case 10:
            switch (MC.sub) {
                case 0:
                    if (MC.card[MC.cur].free >= 350) {
                        MC.sub = 1;
                    } else {
                        MC.err = 7;
                        MC.errCard = MC.cur;
                        MC.state = 0;
                        MC.cur = 0;
                    }
                    MC.busy = 0;
                    break;
                case 1:
                    if (sce_mc_mkdir(MC.card[MC.cur].port, MC.card[MC.cur].slot, mc_data_0013D2D0) == 0) {
                        MC.sub = 2;
                    }
                    break;
                case 2:
                    if (MC.result == 0 || MC.result == -4) {
                        MC.sub = 3;
                        break;
                    }
                    MC.errCard = MC.cur;
                    if (MC.result == -3) {
                        MC.err = 7;
                    } else if (MC.result == -2) {
                        MC.err = 6;
                    } else {
                        MC.err = 0xD;
                    }
                    MC.state = 0;
                    MC.cur = 0;
                    MC.busy = 0;
                    break;
                case 3: {
                    McChunk *chunk;
                    s32 size;

                    mc_call_001FDF10(mc_data_00137C80.x14 << 11, &chunk, &size);
                    mc_call_00217628(chunk, mc_data_00137C80.x10, mc_data_00137C80.x14);
                    MC.sub = 4;
                    MC.busy = 0;
                    break;
                }
                case 4:
                    if (mc_data_001517D8[0] == 0) {
                        MC.sub = 5;
                    }
                    MC.busy = 0;
                    break;
                case 5:
                case 9:
                case 13:
                case 18:
                    switch (MC.sub) {
                        case 5:
                            mc_call_001166FC(workspace, mc_data_0013D300);
                            break;
                        case 9:
                            mc_call_001166FC(workspace, mc_data_0013D320);
                            break;
                        case 18:
                            mc_call_001166FC(workspace, mc_data_0013D340);
                            break;
                        case 13:
                            sprintf(workspace, mc_data_0013D370, MC.card[MC.cur].x14);
                            break;
                    }
                    if (mc_call_001238B0(MC.card[MC.cur].port, MC.card[MC.cur].slot, workspace, 0x203) == 0) {
                        MC.fd = -1;
                        MC.sub++;
                    }
                    break;
                case 6:
                case 10:
                case 14:
                case 19: {
                    McChunk *chunk;
                    s32 size;

                    if (MC.result >= 0) {
                        if (MC.fd < 0) {
                            MC.fd = MC.result;
                        }
                        mc_call_001FDF10(mc_data_00137C94[0] << 11, &chunk, &size);
                        switch (MC.sub) {
                            case 6:
                                MC.size = 0x3C4;
                                MC.buf = (u8 *)chunk + chunk->x0;
                                break;
                            case 10:
                                MC.size = chunk->xC;
                                MC.buf = (u8 *)chunk + chunk->x8;
                                break;
                            case 19:
                                if (mc_data_0015EF90 != 0) {
                                    MC.size = 0x3C00;
                                } else {
                                    MC.size = 0x3C04;
                                }
                                MC.buf = &mc_data_0015EE84;
                                break;
                            case 14: {
                                s32 n = mc_call_0020BAD8(mc_data_001A05C0);
                                MC.size = n + mc_call_0020BAD8(mc_data_001A08C0) * 20 + 8;
                                MC.buf = (u8 *)chunk + chunk->x10;
                                break;
                            }
                        }
                        if (sce_mc_write_buffer(MC.fd, MC.buf, MC.size) == 0) {
                            MC.sub++;
                        }
                    } else {
                        MC.err = 0xA;
                        MC.errCard = MC.cur;
                        MC.state = 0;
                        MC.cur = 0;
                        MC.busy = 0;
                    }
                    break;
                }
                case 7:
                case 11:
                case 15:
                case 20:
                    if (MC.result == MC.size) {
                        if (sce_mc_close(MC.fd) == 0) {
                            MC.sub++;
                        }
                        break;
                    }
                    MC.errCard = MC.cur;
                    if (MC.result >= 0) {
                        MC.err = 0xB;
                        if (sce_mc_close(MC.fd) == 0) {
                            MC.state = 0;
                            MC.cur = 0;
                            MC.busy = 0;
                        }
                        break;
                    }
                    if (MC.result == -4) {
                        MC.err = 8;
                    } else if (MC.result == -3) {
                        MC.err = 7;
                    } else if (MC.result == -2) {
                        MC.err = 6;
                    } else {
                        MC.err = 0xD;
                    }
                    MC.state = 0;
                    MC.cur = 0;
                    MC.busy = 0;
                    break;
                case 8:
                case 12:
                case 16:
                    if (MC.result == 0) {
                        MC.sub++;
                    } else {
                        MC.err = 0xC;
                        MC.errCard = MC.cur;
                        MC.state = 0;
                        MC.cur = 0;
                    }
                    MC.busy = 0;
                    break;
                case 17:
                    if (++MC.card[MC.cur].x14 < 5) {
                        MC.sub = 13;
                    } else {
                        MC.sub++;
                    }
                    MC.busy = 0;
                    break;
                case 21:
                    MC.card[0].x14 = -1;
                    MC.state = 0;
                    MC.cur = 0;
                    MC.card[0].xAC = 0;
                    break;
            }
            break;

        case 11:
            MC.state = 12;
            MC.sub = 0;
            MC.card[MC.cur].x14 = -2;
            /* fall through */
        case 12:
            switch (MC.sub) {
                case 0:
                    if (sce_mc_get_dir(MC.card[MC.cur].port, MC.card[MC.cur].slot, mc_data_0013D2E8, 0, 1,
                                       mc_data_001A0980) == 0) {
                        MC.sub = 1;
                    }
                    break;
                case 1:
                    if (MC.result == 1) {
                        sprintf(workspace, D_E1_0015FE58, mc_data_0013D2D0, mc_data_001A09A0);
                        mc_call_001E9730(D_E1_0015FE60, workspace);
                        if (sce_mc_delete(MC.card[MC.cur].port, MC.card[MC.cur].slot, workspace) == 0) {
                            MC.sub = 2;
                        }
                    } else {
                        if (MC.result == 0) {
                            MC.sub = 3;
                        } else {
                            MC.errCard = MC.cur;
                            if (MC.result == -5) {
                                MC.err = 0x10;
                            } else if (MC.result == -4) {
                                MC.err = 0xF;
                            } else if (MC.result == -2) {
                                MC.err = 0xE;
                            } else {
                                MC.err = 0x12;
                            }
                            MC.state = 0;
                            MC.cur = 0;
                        }
                        MC.busy = 0;
                    }
                    break;
                case 2:
                    if (sce_mc_get_dir(MC.card[MC.cur].port, MC.card[MC.cur].slot, mc_data_0013D2E8, 1, 1,
                                       mc_data_001A0980) == 0) {
                        MC.sub = 1;
                    }
                    break;
                case 3:
                    if (sce_mc_delete(MC.card[MC.cur].port, MC.card[MC.cur].slot, mc_data_0013D2D0) == 0) {
                        mc_call_001E9730(mc_data_001E8518, mc_data_0013D2D0);
                        MC.sub = 4;
                    }
                    break;
                case 4:
                    if (MC.result != 0) {
                        MC.errCard = MC.cur;
                        if (MC.result == -6) {
                            MC.err = 0x11;
                        } else if (MC.result == -5) {
                            MC.err = 0x10;
                        } else if (MC.result == -4) {
                            MC.err = 0xF;
                        } else if (MC.result == -2) {
                            MC.err = 0xE;
                        } else {
                            MC.err = 0x12;
                        }
                    }
                    MC.state = 0;
                    MC.cur = 0;
                    MC.busy = 0;
                    break;
            }
            break;

        case 21:
            MC.state = 22;
            MC.card[MC.cur].x18 = -1;
            MC.xC8 = 0;
            break;

        case 22:
            MC.card[MC.cur].x18++;
            if (MC.card[MC.cur].x18 < 5) {
                MC.sub = 0;
                MC.state = 23;
            } else {
                MC.state = 0;
                MC.cur = 0;
                MC.busy = 0;
            }
            break;
        case 13:
            if (MC.card[MC.cur].x14 < 0) {
                MC.err = 0x13;
                MC.state = 0;
                MC.cur = 0;
                MC.busy = 0;
                break;
            }
            MC.xC8 = 0;
            MC.sub = 0;
            MC.state = 14;
            /* fall through */
        case 14:
        case 23:
            switch (MC.sub) {
                case 0:
                    if (MC.state == 23) {
                        sprintf(workspace, mc_data_0013D370, MC.card[MC.cur].x18);
                    } else {
                        sprintf(workspace, mc_data_0013D370, MC.card[MC.cur].x14);
                    }
                    if (mc_call_001238B0(MC.card[MC.cur].port, MC.card[MC.cur].slot, workspace, 1) == 0) {
                        MC.sub++;
                    }
                    break;
                case 1:
                    if (MC.result >= 0) {
                        MC.fd = MC.result;
                        MC.sub = 2;
                    } else {
                        MC.errCard = MC.cur;
                        if (MC.result == -7) {
                            MC.err = 0x14;
                        } else if (MC.result == -5) {
                            MC.err = 0x15;
                        } else if (MC.result == -4) {
                            MC.err = 0x16;
                        } else if (MC.result == -3) {
                            MC.err = 0x17;
                        } else if (MC.result == -2) {
                            MC.err = 0x18;
                        } else {
                            MC.err = 0x1C;
                        }
                        MC.state = 0;
                        MC.cur = 0;
                    }
                    MC.busy = 0;
                    break;
                case 2:
                    MC.size = 8;
                    if (sce_mc_read(MC.fd, &MC.card[MC.cur].xB0, 8) == 0) {
                        MC.sub = 3;
                    }
                    break;
                case 3:
                    if (MC.result == MC.size) {
                        MC.busy = 0;
                        MC.sub = 4;
                        break;
                    }
                    MC.errCard = MC.cur;
                    if (MC.result >= 0) {
                        MC.err = 0x1B;
                        if (sce_mc_close(MC.fd) == 0) {
                            MC.state = 0;
                            MC.cur = 0;
                            MC.busy = 0;
                        }
                        break;
                    }
                    if (MC.result == -5) {
                        MC.err = 0x15;
                    } else if (MC.result == -4) {
                        MC.err = 0x19;
                    } else if (MC.result == -3) {
                        MC.err = 0x1A;
                    } else if (MC.result == -2) {
                        MC.err = 0x18;
                    } else {
                        MC.err = 0x1C;
                    }
                    MC.state = 0;
                    MC.cur = 0;
                    MC.busy = 0;
                    break;
                case 4:
                    MC.size = MC.card[MC.cur].xB0;
                    if (MC.size > 0x1800) {
                        mc_call_001F9978();
                    }
                    if (sce_mc_read(MC.fd, mc_data_0014EFD0, MC.size) == 0) {
                        MC.sub = 5;
                    }
                    break;
                case 5:
                    if (MC.result == MC.size) {
                        if (MC.state == 23) {
                            mc_call_0020BCB0(mc_data_0014EFD0, MC.cur, MC.card[MC.cur].x18);
                            MC.sub = 8;
                        } else {
                            MC.card[MC.cur].xAC = mc_call_0020BD70(mc_data_0014EFD0, 0, mc_data_001A05C0);
                            MC.sub = 6;
                        }
                        MC.busy = 0;
                        break;
                    }
                    MC.errCard = MC.cur;
                    if (MC.result >= 0) {
                        MC.err = 0x1B;
                        if (sce_mc_close(MC.fd) == 0) {
                            MC.state = 0;
                            MC.cur = 0;
                            MC.busy = 0;
                        }
                        break;
                    }
                    if (MC.result == -5) {
                        MC.err = 0x15;
                    } else if (MC.result == -4) {
                        MC.err = 0x19;
                    } else if (MC.result == -3) {
                        MC.err = 0x1A;
                    } else if (MC.result == -2) {
                        MC.err = 0x18;
                    } else {
                        MC.err = 0x1C;
                    }
                    MC.state = 0;
                    MC.cur = 0;
                    MC.busy = 0;
                    break;
                case 6:
                    MC.size = MC.card[MC.cur].xB4;
                    if (MC.size > 0x1000) {
                        mc_call_001F9978();
                    }
                    if (sce_mc_read(MC.fd, (u8 *)D_E1_001506D0, MC.size) == 0) {
                        MC.sub = 7;
                    }
                    break;
                case 7:
                    if (MC.result == MC.size) {
                        MC.card[MC.cur].xAC += mc_call_0020BD70((u8 *)D_E1_001506D0, MC.xC8, mc_data_001A08C0);
                        if (++MC.xC8 < 20) {
                            MC.sub = 6;
                        } else {
                            MC.sub = 8;
                        }
                        MC.busy = 0;
                        break;
                    }
                    MC.errCard = MC.cur;
                    if (MC.result >= 0) {
                        MC.err = 0x1B;
                        if (sce_mc_close(MC.fd) == 0) {
                            MC.state = 0;
                            MC.cur = 0;
                            MC.busy = 0;
                        }
                        break;
                    }
                    if (MC.result == -5) {
                        MC.err = 0x15;
                    } else if (MC.result == -4) {
                        MC.err = 0x19;
                    } else if (MC.result == -3) {
                        MC.err = 0x1A;
                    } else if (MC.result == -2) {
                        MC.err = 0x18;
                    } else {
                        MC.err = 0x1C;
                    }
                    MC.state = 0;
                    MC.cur = 0;
                    MC.busy = 0;
                    break;
                case 8:
                    if (sce_mc_close(MC.fd) == 0) {
                        if (MC.state == 23) {
                            MC.busy = 0;
                            MC.state = 22;
                        } else {
                            MC.state = 0;
                            MC.cur = 0;
                        }
                    }
                    break;
            }
            break;

        case 15:
            if (MC.card[MC.cur].xAC != 0) {
                MC.busy = 0;
                MC.state = 0;
                MC.cur = 0;
                MC.err = 0x2717;
                break;
            }
            if (MC.card[MC.cur].x14 < 0) {
                MC.busy = 0;
                MC.state = 0;
                MC.cur = 0;
                MC.err = 0x1D;
                break;
            }
            MC.sub = 0;
            MC.state = 16;
            /* fall through */
        case 16:
            switch (MC.sub) {
                case 0:
                    sprintf(workspace, mc_data_0013D370, MC.card[MC.cur].x14);
                    if (mc_call_001238B0(MC.card[MC.cur].port, MC.card[MC.cur].slot, workspace, 2) == 0) {
                        MC.sub = 1;
                    }
                    break;
                case 1:
                    if (MC.result >= 0) {
                        MC.fd = MC.result;
                        MC.sub = 2;
                    } else {
                        MC.errCard = MC.cur;
                        if (MC.result == -7) {
                            MC.err = 0x1E;
                        } else if (MC.result == -5) {
                            MC.err = 0x1F;
                        } else if (MC.result == -4) {
                            MC.err = 0x20;
                        } else if (MC.result == -3) {
                            MC.err = 0x21;
                        } else if (MC.result == -2) {
                            MC.err = 0x22;
                        } else {
                            MC.err = 0x26;
                        }
                        MC.state = 0;
                        MC.cur = 0;
                    }
                    MC.busy = 0;
                    break;
                case 2:
                    if (sce_mc_seek(MC.fd, 8, 0) == 0) {
                        MC.sub = 3;
                    }
                    break;
                case 3:
                    if (MC.result >= 0) {
                        MC.sub = 4;
                    } else {
                        MC.state = 0;
                        MC.cur = 0;
                        MC.busy = 0;
                    }
                    break;
                case 4:
                    if (sce_mc_write_buffer(MC.fd, mc_data_0014EFD0, mc_call_0020BAD8(mc_data_001A05C0)) == 0) {
                        MC.sub = 5;
                    }
                    break;
                case 5:
                    if (MC.result == mc_call_0020BAD8(mc_data_001A05C0)) {
                        MC.sub = 6;
                        break;
                    }
                    MC.errCard = MC.cur;
                    if (MC.result >= 0) {
                        MC.err = 0x25;
                        if (sce_mc_close(MC.fd) == 0) {
                            MC.state = 0;
                            MC.cur = 0;
                            MC.busy = 0;
                        }
                        break;
                    }
                    if (MC.result == -5) {
                        MC.err = 0x1F;
                    } else if (MC.result == -4) {
                        MC.err = 0x23;
                    } else if (MC.result == -3) {
                        MC.err = 0x24;
                    } else if (MC.result == -2) {
                        MC.err = 0x22;
                    } else {
                        MC.err = 0x26;
                    }
                    MC.state = 0;
                    MC.cur = 0;
                    MC.busy = 0;
                    break;
                case 6:
                    if (MC.xC8 == 0) {
                        MC.sub = 8;
                        break;
                    }
                    if (sce_mc_seek(MC.fd, MC.xC8 * mc_call_0020BAD8(mc_data_001A08C0), 1) == 0) {
                        MC.sub = 7;
                    }
                    break;
                case 7:
                    if (MC.result >= 0) {
                        MC.sub = 8;
                    } else {
                        MC.state = 0;
                        MC.cur = 0;
                        MC.busy = 0;
                    }
                    break;
                case 8:
                    if (sce_mc_write_buffer(MC.fd, (u8 *)D_E1_001506D0, mc_call_0020BAD8(mc_data_001A08C0)) == 0) {
                        MC.sub = 9;
                    }
                    break;
                case 9:
                    if (MC.result == mc_call_0020BAD8(mc_data_001A08C0)) {
                        MC.sub = 10;
                        break;
                    }
                    MC.errCard = MC.cur;
                    if (MC.result >= 0) {
                        MC.err = 0x25;
                        if (sce_mc_close(MC.fd) == 0) {
                            MC.state = 0;
                            MC.cur = 0;
                            MC.busy = 0;
                        }
                        break;
                    }
                    if (MC.result == -5) {
                        MC.err = 0x1F;
                    } else if (MC.result == -4) {
                        MC.err = 0x23;
                    } else if (MC.result == -3) {
                        MC.err = 0x24;
                    } else if (MC.result == -2) {
                        MC.err = 0x18;
                    } else {
                        MC.err = 0x1C;
                    }
                    MC.state = 0;
                    MC.cur = 0;
                    MC.busy = 0;
                    break;
                case 10:
                    if (sce_mc_close(MC.fd) == 0) {
                        MC.state = 0;
                        MC.cur = 0;
                    }
                    break;
            }
            break;

        case 17: {
            McChunk *chunk;
            s32 size;

            mc_call_001FDF10(mc_data_00137C80.x14 << 11, &chunk, &size);
            mc_call_00217628(chunk, mc_data_00137C80.x10, mc_data_00137C80.x14);
            MC.sub = 18;
            MC.busy = 0;
            break;
        }

        case 18:
            if (mc_data_001517D8[0] == 0) {
                McChunk *chunk;
                s32 size;

                mc_call_001FDF10(mc_data_00137C94[0] << 11, &chunk, &size);
                MC.sub = 19;
                MC.buf = (u8 *)chunk + chunk->x10;
            }
            MC.busy = 0;
            break;

        case 19:
            if (MC.card[MC.cur].xAC != 0) {
                MC.busy = 0;
                MC.state = 0;
                MC.cur = 0;
                MC.err = 0x2718;
                break;
            }
            if (MC.card[MC.cur].x14 < 0) {
                MC.busy = 0;
                MC.state = 0;
                MC.cur = 0;
                MC.err = 0x27;
                break;
            }
            MC.xC8 = 0;
            MC.sub = 0;
            MC.state = 20;
            /* fall through */
        case 20:
            switch (MC.sub) {
                case 0:
                    sprintf(workspace, mc_data_0013D370, MC.card[MC.cur].x14);
                    if (mc_call_001238B0(MC.card[MC.cur].port, MC.card[MC.cur].slot, workspace, 2) == 0) {
                        MC.sub = 1;
                    }
                    break;
                case 1:
                    if (MC.result >= 0) {
                        s32 n;

                        MC.fd = MC.result;
                        n = mc_call_0020BAD8(mc_data_001A05C0);
                        MC.size = n + mc_call_0020BAD8(mc_data_001A08C0) * 20 + 8;
                        if (sce_mc_write_buffer(MC.fd, MC.buf, MC.size) == 0) {
                            MC.sub = 2;
                        }
                        break;
                    }
                    MC.err = 0x2B;
                    MC.errCard = MC.cur;
                    MC.state = 0;
                    MC.cur = 0;
                    MC.busy = 0;
                    break;
                case 2:
                    if (MC.result == MC.size) {
                        if (sce_mc_close(MC.fd) == 0) {
                            MC.sub = 3;
                        }
                        break;
                    }
                    MC.errCard = MC.cur;
                    if (MC.result >= 0) {
                        MC.err = 0xB;
                        if (sce_mc_close(MC.fd) == 0) {
                            MC.state = 0;
                            MC.cur = 0;
                            MC.busy = 0;
                        }
                        break;
                    }
                    if (MC.result == -4) {
                        MC.err = 0x28;
                    } else if (MC.result == -3) {
                        MC.err = 0x29;
                    } else if (MC.result == -2) {
                        MC.err = 0x2A;
                    } else {
                        MC.err = 0x2D;
                    }
                    MC.state = 0;
                    MC.cur = 0;
                    MC.busy = 0;
                    break;
                case 3:
                    if (MC.result != 0) {
                        MC.err = 0x2C;
                        MC.errCard = MC.cur;
                    }
                    MC.state = 0;
                    MC.cur = 0;
                    MC.busy = 0;
                    break;
            }
            break;
    }
}
