#include "common.h"

/* Ground-material portion of the shared hero input handler. */

typedef struct {
    unsigned char pad0[136];
    float positionZ;
    unsigned char pad8c[140];
    float displacementZ;
    unsigned char pad11c[194];
    short inputLock;
    unsigned char pad1e0[312];
    float groundZ;
    float groundDistance;
    unsigned char pad320[16];
    float waterSurfaceZ;
    unsigned char pad334[12];
    int groundedFrames;
    unsigned char pad344[4];
    short pendingJump;
    unsigned char pad34a[2];
    short framesOffGround;
    unsigned char pad34e[4418];
    short groundType;
    unsigned char onIceGround;
    unsigned char pad1493[1];
    unsigned char baseCondition;
    unsigned char pad1495[2];
    unsigned char onMagnetGround;
    unsigned char pad1498[2];
    unsigned char onSlipGround;
    unsigned char pad149b[1];
    unsigned char sinkGround;
    unsigned char groundSoundClass;
    unsigned char pad149e[3574];
    int moveState;
    unsigned char pad2298[28];
    unsigned char vehicleMode;
    unsigned char pad22b5[4];
    unsigned char wading;
    unsigned char pad22ba[3];
    unsigned char disabled;
    unsigned char pad22be[518];
    float waterDepth;
} HeroGroundFlags;
extern HeroGroundFlags D_00189E20;
extern unsigned char D_001A7B0B __attribute__((sda));
extern void func_00282A88(void *, int);
extern float func_00282C48(float);
extern void func_L04_002E7C08(int, int);

void func_L04_002D38E8(void)
{
    short state = D_00189E20.groundType;
    unsigned char saved = D_00189E20.groundSoundClass;
    func_00282A88(&D_00189E20.groundType, 16);
    D_00189E20.groundSoundClass = saved;
    D_00189E20.groundType = state;
    D_00189E20.wading = 0;
    D_00189E20.pendingJump = 0;
    if (D_00189E20.disabled) return;
    /* Retained eligibility calculation from the corresponding common input
     * handler, func_L00_002ACA08. Its result is unused in this reduced variant;
     * the compiler retains the groundedFrames read found in retail. */
    {
        int active = 0;
        if (D_00189E20.groundedFrames ||
            (D_00189E20.moveState == 123 && D_00189E20.groundDistance < 0.4f))
            active = 1;
    }
    if (state == -1) return;
    if (state == 2 && D_00189E20.inputLock == 0 &&
        (D_00189E20.framesOffGround == 0 || D_00189E20.groundDistance < 2.5f)) {
        D_00189E20.onMagnetGround = 1;
        if (D_00189E20.vehicleMode == 0 && D_001A7B0B)
            D_00189E20.pendingJump = 1;
    }
    if (state == 14) {
        D_00189E20.baseCondition = 1;
        D_00189E20.waterDepth = 0.2f;
        D_00189E20.waterSurfaceZ = D_00189E20.groundZ + 0.2f;
    }
    if (state == 0) {
        float remaining = D_00189E20.waterSurfaceZ - D_00189E20.groundZ;
        D_00189E20.waterDepth = remaining;
        if (remaining < 0.85f && remaining > 0.25f)
            D_00189E20.wading = 1;
        D_00189E20.baseCondition = 1;
    }
    if (state == 7) D_00189E20.onIceGround = 1;
    if (state == 13) D_00189E20.sinkGround = 1;
    if (state == 8) D_00189E20.onSlipGround = 1;
    if (state == 12) D_00189E20.onSlipGround = 1;
    if (D_00189E20.sinkGround && D_00189E20.moveState != 124 &&
        func_00282C48(D_00189E20.waterSurfaceZ - (D_00189E20.positionZ + 0.25f)) < 1.0f &&
        D_00189E20.waterSurfaceZ - D_00189E20.positionZ > 0.0f &&
        (D_00189E20.displacementZ < 0.0f || D_00189E20.moveState == 123))
        func_L04_002E7C08(124, 1);
}

