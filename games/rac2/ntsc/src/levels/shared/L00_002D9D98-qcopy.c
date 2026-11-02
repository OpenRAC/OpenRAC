/* func_L00_002D9D98: selects the default camera and copies its position and target. */
typedef int Quad __attribute__((mode(TI)));
typedef struct {
    char pad0[0x140];
    union { Quad q; float f[4]; } position;
    char pad150[0x40];
    char *active;
    char pad194[0xc];
    float target[3];
    char pad1ac[0xe4];
    short state;
    char pad292[0x172];
    int flags;
} CameraState;
extern CameraState D_L00_001B8C80;
extern char D_L00_001BB1B0[];
extern float D_00189EA0[3];
extern char *func_00270280(int);
extern void func_00271B90(void);
extern void func_L00_002D7D58(char *);
extern void func_002701B0(void);
extern void func_L00_002D75F0(void);

void func_L00_002D9D98(void)
{
    CameraState *s = &D_L00_001B8C80;
    if (s->active) {
        *(short *)(s->active+0x8e)=0;
        s->active[0x7d]=0;
        *(short *)(s->active+0x7e)=0;
    }
    s->active=func_00270280(0);
    *(short *)(s->active+0x8e)=1;
    *(char **)(s->active+0x70)=D_L00_001BB1B0;
    s->active[0x7d]=0;
    s->state=0;
    func_00271B90();
    s->flags=0;
    func_L00_002D7D58(s->active);
    func_002701B0();
    { char *active=s->active;
      float *position=s->position.f;
      *(Quad *)position=*(Quad *)(active+0x30);
      { float *out=(float *)(active+0x64);
        out[0]=position[0]; out[1]=position[1]; out[2]=position[2];
      }
      { float *target=s->target; float *origin=D_00189EA0;
        target[0]=origin[0]; target[1]=origin[1]; target[2]=origin[2];
      }
    }
    func_L00_002D75F0();
}
