/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game/rac1-pal/hand/level_vendor_draw.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%d: %s\n", __LINE__, #c); exit(1); } } while (0)
_Static_assert(sizeof(VendorQuad) == 0x90, "complete retail draw record");
_Static_assert(offsetof(VendorQuad, st) == 0x50, "ST offset");
_Static_assert(offsetof(VendorQuad, texture) == 0x78, "64-bit TEX0 offset");
int vendor_blend[7];
float vendor_camera[0x54], vendor_vertices[103][4], vendor_normals[103][4];
float vendor_world[103][4], vendor_uv[103][2], vendor_saved_uv[103][2];
short vendor_indices[74][4][3];
static unsigned char moby[0x100];
static float data[20];
static VendorQuad draws[74];
static int count, transforms, countdowns;
static const unsigned long long tex = 0xFEDCBA9876543210ULL;
unsigned long long vendor_texture(int id) { CHECK(id == 17); return tex; }
float vendor_abs(float v) { return fabsf(v); }
float vendor_sqrt(float v) { return sqrtf(v); }
int vendor_ticks(int n) { CHECK(n == 60); return 72; }
float vendor_float(int n) { return (float)n; }
int vendor_countdown(int* n) { ++countdowns; if (*n != 0) *n = *n > 1 ? *n - 1 : 0; return 0; }
void vendor_transform(float* out, float* v, float* m) {
    float r[4]; CHECK(m == data); ++transforms;
    for (int i = 0; i < 4; ++i) r[i] = v[0]*m[i]+v[1]*m[i+4]+v[2]*m[i+8]+v[3]*m[i+12];
    memcpy(out, r, sizeof(r));
}
void vendor_sub(float* out, float* a, float* b) {
    for (int i = 0; i < 3; ++i) out[i] = a[i] - b[i];
    out[3] = a[3];
}
void vendor_normalize(float* out, float* a, float length) {
    float scale = length / sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
    for (int i = 0; i < 3; ++i) out[i] = a[i] * scale;
    out[3] = a[3];
}
float vendor_dot(float* a, float* b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
void vendor_scale(float* out, float* a, float s) {
    for (int i = 0; i < 3; ++i) out[i] = a[i] * s;
    out[3] = a[3];
}
void vendor_draw(VendorQuad* q, void* matrix, int flags) {
    CHECK(!matrix && !flags && count < 74); draws[count++] = *q;
}
static void reset(void) {
    memset(moby, 0, sizeof(moby)); memset(data, 0, sizeof(data));
    memset(vendor_camera, 0, sizeof(vendor_camera));
    data[0] = data[5] = data[10] = data[15] = 1;
    *(float**)(moby + 0x78) = data;
    moby[0x20] = 2; moby[0xBC] = 0;
    vendor_blend[0]=1; vendor_blend[1]=2; vendor_blend[2]=0;
    vendor_blend[3]=1; vendor_blend[4]=0x80; vendor_blend[5]=17;
    vendor_blend[6]=(int)0x87654321u;
    for (int i = 0; i < 103; ++i) {
        vendor_vertices[i][0]=3.0f+(float)i/100; vendor_vertices[i][1]=0;
        vendor_vertices[i][2]=4; vendor_vertices[i][3]=1;
        vendor_normals[i][0]=vendor_normals[i][1]=vendor_normals[i][3]=0;
        vendor_normals[i][2]=1;
        for (int j=0;j<4;++j) vendor_world[i][j]=-123;
        vendor_uv[i][0]=(float)i; vendor_uv[i][1]=-(float)i;
        vendor_saved_uv[i][0]=0.25f; vendor_saved_uv[i][1]=0.75f;
    }
    for (int i=0;i<74;++i) for (int j=0;j<4;++j) {
        vendor_indices[i][j][0]=(short)((101-i*3-j+306)%102);
        vendor_indices[i][j][1]=-300; vendor_indices[i][j][2]=300;
    }
    count=transforms=countdowns=0;
}
static void packets(unsigned long long alpha) {
    CHECK(count==74);
    for (int i=0;i<74;++i) {
        CHECK(draws[i].texture==tex && draws[i].alpha==alpha);
        CHECK(draws[i].clamp==0 && draws[i].filter==0xFF9000000260ULL);
        for (int j=0;j<4;++j) {
            int k=vendor_indices[i][j][0];
            CHECK(draws[i].color[j]==0x87654321u);
            CHECK(!memcmp(draws[i].corner[j],vendor_world[k],16));
            CHECK(!memcmp(draws[i].st[j],vendor_uv[k],8));
        }
    }
    CHECK(vendor_world[102][0]==-123 && vendor_world[102][3]==-123);
    CHECK(vendor_uv[102][0]==102 && vendor_uv[102][1]==-102);
    CHECK(vendor_saved_uv[102][0]==0.25f && vendor_saved_uv[102][1]==0.75f);
}
/* Independent double-precision reflection/sphere-map reference for the
 * synthetic identity transform, eye at zero and z-axis normal. */
static void reflection(float ratio) {
    for (int i=0;i<102;++i) {
        double x=vendor_vertices[i][0], z=4.0, length=sqrt(x*x+z*z);
        x/=length; z=z/length*0.98;
        length=sqrt(x*x+z*z); x/=length; z/=length;
        double u=x/(2*sqrt(2*(z+1)))+0.5;
        CHECK(fabs(vendor_uv[i][0]-(u+(0.25-u)*ratio))<0.000002);
        CHECK(fabs(vendor_uv[i][1]-(0.5+0.25*ratio))<0.000002);
    }
}
int main(void) {
    const unsigned long long alpha=0x8000000049ULL;
    reset(); *(float*)(moby+0x10)=16; moby[0xBC]=1;
    data[12]=7; func_L00_002D2E60(moby);
    CHECK(!moby[0xBC] && transforms==102 && !countdowns && *(int*)(data+16)==72);
    for(int i=0;i<102;++i) {
        CHECK(vendor_saved_uv[i][0]==(float)i && vendor_saved_uv[i][1]==-(float)i);
        CHECK(vendor_world[i][0]==vendor_vertices[i][0]+7);
    }
    packets(alpha);
    reset(); *(float*)(moby+0x14)=-16; moby[0xBC]=2;
    func_L00_002D2E60(moby); CHECK(moby[0xBC]==2 && !countdowns && transforms==102);
    CHECK(vendor_saved_uv[0][0]==0.25f); packets(alpha);
    reset(); moby[0x20]=1; *(float*)(moby+0x10)=100; *(int*)(data+16)=37;
    func_L00_002D2E60(moby); CHECK(moby[0x20]==2 && moby[0xBC]==1);
    CHECK(*(int*)(data+16)==36 && countdowns==1 && transforms==204);
    reflection(0); packets(alpha);
    reset(); *(int*)(data+16)=37; func_L00_002D2E60(moby);
    reflection(0.5f); packets(alpha);
    reset(); *(int*)(data+16)=1; func_L00_002D2E60(moby);
    CHECK(*(int*)(data+16)==0); reflection(0); packets(alpha);
    reset(); vendor_blend[0]=-1; func_L00_002D2E60(moby);
    reflection(0); packets(~0ULL);
    puts("vendor draw: OK"); return 0;
}
