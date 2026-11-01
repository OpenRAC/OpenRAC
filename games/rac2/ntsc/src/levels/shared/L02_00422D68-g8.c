typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef struct { float vertex[4][4]; u32 color[4]; float uv[8]; u64 reserved,texture,setup,state; } QuadPacket;
extern void ref_L02_00422D68_00(char *);
extern float func_002A7878(float,float);
extern void func_00282D08(void *,void *,void *,float);
extern u64 func_0027C878(int);
extern int func_001163B0(void);
extern int func_002A7820(int,int);
extern float func_00283248(float,float);
extern float func_00282E80(void *,void *);
extern float func_002A7940(void);
extern void func_00283410(void *,void *);
extern void func_00282D30(void *,void *,float);
extern void func_00280B90(void *,void *,int);
extern void func_L02_004230B0(u8 *,void *);
/* Observed target prefix: retail addresses its position at base + 0x140. */
typedef struct { u8 reserved[0x140]; float position[4]; } FlareTargetView;
extern FlareTargetView D_L02_001BA3C0 __attribute__((nosda));
extern u32 D_L02_001B1FDC __attribute__((sda));
extern u32 D_L02_001B1FE0 __attribute__((sda));

/* Draw a randomly displaced point on the beam, then its smaller inner glow. */
void func_L02_00422D68(u8 *object)
{
    QuadPacket packet;
    float transform[16];
    float rotation[4];
    float position[4];
    u8 *state=*(u8 **)(object+0x68);
    float size;
    int alpha;
    u32 color;
    ref_L02_00422D68_00((char *)&packet);
    func_00282D08(position,state+0x10,state+0x20,func_002A7878(0.0f,1.0f));
    position[0]+=func_002A7878(-0.1f,0.1f)**(float *)(state+0x60);
    position[1]+=func_002A7878(-0.1f,0.1f)**(float *)(state+0x60);
    position[2]+=func_002A7878(-0.1f,0.1f)**(float *)(state+0x60);
    packet.texture=func_0027C878(11);
    packet.setup=0xFF9000000260ULL;
    packet.reserved=0;
    packet.state=0x8000000040ULL | ((u64)(func_001163B0()%2+1)<<2);
    alpha=func_002A7820(64,127);
    color=D_L02_001B1FDC;
    color&=0xFFFFFF;
    color+=alpha<<24;
    packet.color[3]=color; packet.color[2]=color; packet.color[1]=color; packet.color[0]=color;
    rotation[3]=0;
    rotation[2]=func_00283248(D_L02_001BA3C0.position[0]-position[0],D_L02_001BA3C0.position[1]-position[1]);
    rotation[1]=-func_00283248(func_00282E80(position,D_L02_001BA3C0.position),D_L02_001BA3C0.position[2]-position[2]);
    rotation[0]=func_002A7940();
    func_00283410(transform,rotation);
    transform[12]=position[0];transform[13]=position[1];transform[14]=position[2];transform[15]=1;
    size=func_002A7878(0.05f,0.15f)**(float *)(state+0x60);
    func_00282D30(packet.vertex[0],packet.vertex[0],size);
    func_00282D30(packet.vertex[1],packet.vertex[1],size);
    func_00282D30(packet.vertex[2],packet.vertex[2],size);
    func_00282D30(packet.vertex[3],packet.vertex[3],size);
    func_00280B90(&packet,transform,0);
    {
        u32 innerColor=(D_L02_001B1FE0&0xFFFFFF)+(alpha<<24);
        packet.color[3]=innerColor;packet.color[2]=innerColor;packet.color[1]=innerColor;packet.color[0]=innerColor;
    }
    func_00282D30(packet.vertex[0],packet.vertex[0],0.5f);
    func_00282D30(packet.vertex[1],packet.vertex[1],0.5f);
    func_00282D30(packet.vertex[2],packet.vertex[2],0.5f);
    func_00282D30(packet.vertex[3],packet.vertex[3],0.5f);
    func_00280B90(&packet,transform,0);
    func_L02_004230B0(object,position);
}
