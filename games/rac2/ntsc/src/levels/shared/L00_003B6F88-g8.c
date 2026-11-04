typedef unsigned int TI128 __attribute__((mode(TI)));
typedef union { TI128 q; float v[4]; } Vector;
extern float D_L00_001AFAE0 __attribute__((sda));
extern void func_00282CF0(void *,void *,void *);
extern float func_00282DF0(void *),func_00282DB0(void *,void *);
extern void func_00282D30(void *,void *,float),func_00283028(void *,void *,void *);
extern void func_L00_003B7190(float *,float *);
extern void *func_002A9640(void *,unsigned int,int);
extern void func_L00_003B7680(void *,void *);
typedef struct {
    unsigned char *actors[2];
    unsigned char reserved08[8];
    Vector positions[2];
    Vector velocities[2];
    unsigned char reserved50[8];
    int scores[2];
    int phase;
} PairContext;
void func_L00_003B6F88(unsigned char *object)
{
    Vector difference, opposite;
    PairContext *context=*(PairContext **)(object+0x68);
    int i;
    if(context->phase==0) {
        func_00282CF0(&difference,&context->positions[0],&context->positions[1]);
        if(func_00282DF0(&difference)<D_L00_001AFAE0+D_L00_001AFAE0) {
            func_00282D30(&opposite,&difference,-1.0f);
            if(func_00282DB0(&difference,&context->velocities[1])>0.0f)
                func_00283028(&context->velocities[1],&context->velocities[1],&opposite);
            if(func_00282DB0(&difference,&context->velocities[0])<0.0f)
                func_00283028(&context->velocities[0],&context->velocities[0],&difference);
        }
    }
    for(i=0;i<2;++i) {
        func_L00_003B7190(&context->positions[i].v[1],&context->positions[i].v[2]);
        if(*(unsigned short *)(context->actors[i]+0x34)&1) continue;
        if(context->positions[i].v[1]*context->positions[i].v[1]+
            context->positions[i].v[2]*context->positions[i].v[2]<D_L00_001AFAE0 ||
            func_002A9640(context->actors[i],0x10000,0)) {
            context->phase=i+1;
            *(unsigned short *)(context->actors[i]+0x34)|=1;
            context->actors[i][0x31]=0;
            if(*(unsigned short *)(context->actors[(i+1)&1]+0x34)&1) --context->scores[i];
            else ++context->scores[(i+1)&1];
            func_L00_003B7680(context->actors[i],context->actors[i]+0x10);
        }
    }
}
