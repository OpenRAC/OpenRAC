/* func_002D0930 in its own unit, a sibling of func_002D01D0: the cursor and the D_001A7478 write
   go through $gp, so the file is compiled at -G32 (config/cflags.txt). */

typedef struct { char prefix[0x14]; char *source; int value; char gap[0x118]; int fallback; } SelectionCallback;

typedef struct { char reserved[0x1C4]; int flags; } CallbackMenuState;

extern SelectionCallback D_001F2840;
extern CallbackMenuState D_00138180;
extern int func_002E5E58(int, unsigned int, void *);
extern int D_001ABA20 __attribute__((sda));
extern short D_001A63AC[];
typedef struct { char reserved00[0x10]; int flags; } FlagTarget;
typedef struct { char reserved00[0x44]; int f44; int f48; int f4c; int f50; int f54; int f58; } MenuFields;

extern int D_001A7478_sda __attribute__((sda));
extern int D_001A7478;
extern char D_001BAD40[];
extern void func_002C9E60(int);
extern int func_00288AA0(char *, int);
extern int func_00288BE8(char *);
extern MenuFields D_0025C3E8;
extern FlagTarget *D_0025C3A8[];
typedef struct { short f0; short text; short enabled; } MenuEntry;
extern MenuEntry D_00261858[];
typedef struct { char p00[8]; int f08; char p0c[8]; int f14; int f18; char pad[0xE4]; int f100; int f104; char pad2[0x38]; int f140; } GameObj;


int func_002D0930(void)
{
    int result = 0;
    int flags = ((CallbackMenuState *)&D_00138180)->flags;
    int old = D_001ABA20;

    if (flags & 0x10) {
        int value;
        int status;
        D_001ABA20 = 0;
        value = *(int *)(D_001F2840.source + 0xE0);
        if (value != 0)
            D_001F2840.value = value;
        else if (D_001F2840.fallback == 0) {
            status = -1;
            goto finish;
        }
        status = 0;
finish:
        return status;
    }
    if (flags & 0x900) {
        D_001ABA20 = 0;
        result = 1;
    } else {
        if (flags & 0x1000) {
            func_002E5E58(3, 0, 0);
            if (--D_001ABA20 < 0)
                D_001ABA20 = 0x20;
        } else if (flags & 0x4000) {
            func_002E5E58(3, 0, 0);
            if (++D_001ABA20 >= 0x21)
                D_001ABA20 = 0;
        }
        if (((CallbackMenuState *)&D_00138180)->flags & 0x40) {
            if (D_00261858[D_001ABA20].enabled != 0) {
                char *obj;
                func_002E5E58(4, 0, 0);
                func_002C9E60(0);
                ((GameObj *)&D_001F2840)->f100 = ((GameObj *)&D_001F2840)->f14;
                ((GameObj *)&D_001F2840)->f104 = ((GameObj *)&D_001F2840)->f08;
                ((GameObj *)&D_001F2840)->f140 = D_001A7478;
                D_001A7478_sda = 2;
                obj = D_001BAD40;
                func_00288AA0(obj, D_00261858[D_001ABA20].text);
                func_00288BE8(obj);
            } else
                func_002E5E58(5, 0, 0);
        }
        {
            MenuFields *menu = &D_0025C3E8;
            menu->f58 = D_001ABA20;
            if (D_00261858[D_001ABA20].enabled != 0) {
                if (old == D_001ABA20) {
                    if (D_001A63AC[0] == 0) {
                        if (menu->f50 == old && menu->f44 == 2) {
                            D_0025C3A8[0]->flags &= -5;
                        } else if (menu->f54 == old && menu->f44 == 4) {
                            D_0025C3A8[0]->flags &= -5;
                        }
                    }
                }
            } else {
                D_0025C3A8[0]->flags |= 4;
            }
        }
    }
    return result;
}
