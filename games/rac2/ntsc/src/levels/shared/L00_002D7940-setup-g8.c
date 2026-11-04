#include "common.h"

typedef struct { float x,y,z,w; } CameraVec9E;
typedef struct {
    char pad00[0x20];
    CameraVec9E up, target, previous, velocity, hero;
    char pad70[0xac-0x70];
    float yaw0, yaw1, yaw2, yaw3, yaw4;
    int c0, c4, c8, cc;
    char padd0[0xe4-0xd0];
    int e4, e8, ec;
} CameraFollow9E;
typedef struct {
    short phase; char pad002[0x110-2]; int unknown[5];
} CameraBlend9E;
typedef struct {
    char pad000[0x194];
    int previous;
    char pad198[8];
    CameraFollow9E follow;
    CameraBlend9E blend;
    char pad3b4[12];
    CameraVec9E clear;
    char pad3d0[0x30];
    int frames, underwater;
} CameraState9E;
typedef struct {
    char pad00[0x74]; int activation; float blend_time;
    u8 priority, handoff; short transition, field80, field82, index, id;
    char field88; u8 hit; short snap, type, hard_cut;
    char pad90[16];
} CameraSlot9E;
typedef struct {
    int field0; CameraSlot9E *slot;
    char pad08[0x1c-8]; u8 priority, field1d, field1e, activation;
} CameraInfo9E;
typedef struct {
    char pad00[12]; unsigned short id; char pad0e[14]; CameraInfo9E *info;
} CameraDef9E;
typedef struct {
    char pad00[0x80]; CameraVec9E pos;
    char pad90[8]; float yaw;
    char pad9c[0x2290-0x9c]; char *object;
} CameraHero9E;
extern CameraState9E D_L00_001B8C80;
extern CameraSlot9E D_L00_001B91D0[];
extern int D_L00_001BB930[];
extern CameraHero9E D_00189E20;
extern float D_001A8900 __attribute__((sda));
extern int D_L00_001B1840 MACRO_ADDR, D_L00_001B1848 MACRO_ADDR;
extern CameraDef9E *D_L00_001B1844 MACRO_ADDR;
extern void func_L00_002D7068(void),func_L00_002D75F0(void),func_L00_002D9D98(void);
extern int func_L00_002D9D48(int);
extern void func_L00_002EE3D0(void *,void *,float);
extern void func_L00_002EE0D0(void *);

/* SetupLevelCameras: the layout and initialization sequence correspond to RAC1's
   setup_level_cameras (rac1-decomp src/overlays/shared/level_camera_setup.c, GPL-3.0),
   with RAC2 offsets measured from the retail body. As in RAC1 the second block takes a
   fresh state pointer (g1). The descriptor address is summed as an integer, offset
   first, which is the operand order retail's addu has without RAC1's -fno-force-mem. */
void func_L00_002D9E80(void) {
    CameraState9E *g = &D_L00_001B8C80;
    CameraState9E *g1;
    CameraFollow9E *q;
    CameraVec9E *up;
    CameraHero9E *hero;
    int i,k;
    g->frames=0;
    g->underwater=0;
    D_001A8900=0.85f;
    D_L00_001B1840=0;
    for(k=0;k<48;k++) {
        D_L00_001B91D0[k].hard_cut=0;
        D_L00_001BB930[k]=0;
    }
    func_L00_002D7068();
    func_L00_002D75F0();
    g1=&D_L00_001B8C80;
    g1->previous=0;
    g1->blend.phase=0;
    qzero_nc(&g1->clear);
    q=&D_L00_001B8C80.follow;
    hero=&D_00189E20;
    up=&q->up;
    func_L00_002EE3D0(up,hero->object+0xe0,1.0f);
    qcopy(&q->target,up);
    qcopy(&q->previous,up);
    func_L00_002EE0D0(&q->velocity);
    qcopy(&q->hero,&hero->pos);
    {
        float yaw=hero->yaw;
        q->c4=0;
        q->e4=0;
        q->c0=0;
        q->cc=0;
        q->e8=0;
        q->yaw4=yaw;
        q->yaw3=yaw;
        q->yaw2=yaw;
        q->yaw1=yaw;
        q->yaw0=yaw;
    }
    i=0;
    if(D_L00_001B1848>0) do {
        int off=i*32;
        CameraDef9E *source=(CameraDef9E *)(off+(int)D_L00_001B1844);
        CameraInfo9E *object=source->info;
        CameraSlot9E *entry=&D_L00_001B91D0[i];
        int id;
        entry->index=i;
        object->slot=entry;
        entry->hard_cut=0;
        entry->hit=0;
        entry->snap=0;
        entry->field82=-1;
        entry->field80=-1;
        entry->handoff=0;
        entry->transition=0;
        entry->blend_time=-1.0f;
        entry->priority=object->priority;
        entry->activation=object->activation;
        id=source->id;
        D_L00_001BB930[i]=1;
        entry->id=id;
        entry->type=func_L00_002D9D48((short)id);
        i++;
    } while(i<D_L00_001B1848);
    func_L00_002D9D98();
    {
        CameraBlend9E *b=&D_L00_001B8C80.blend;
        int *u=b->unknown;
        b->phase=0;
        u[0]=0;
        u[3]=0;
        u[1]=0;
        u[4]=0;
    }
}
