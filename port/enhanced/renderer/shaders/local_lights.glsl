// SPDX-License-Identifier: GPL-3.0-or-later
// The default count zero retains the original uniform bank in isolated shader fixtures.
#define OPENRAC_FORWARD_PLUS 1
uniform samplerBuffer forward_lights;
uniform usamplerBuffer forward_masks;
uniform int forward_count,forward_tiled,forward_transparent;
uniform vec4 forward_view;
// Soft inverse-square shaping, normalized to the game's brightness at half range.
// The fourth-power window reaches zero with zero slope at the original light radius.
float forward_falloff(float distance,float radius) {
    float x=distance/radius,x2=x*x;
    float window=max(0.,1.-x2*x2);
    return window*window/(1.+3.03125*x2);
}
#define LOCAL_ATTENUATION(d,r) forward_falloff(d,r)
int forward_total() { return forward_count>0?forward_count:8; }
vec4 forward_position(int i) { return forward_count>0?texelFetch(forward_lights,i*4):probe_point_position[i]; }
vec4 forward_colour(int i) { return forward_count>0?texelFetch(forward_lights,i*4+1):probe_point_colour[i]; }
float forward_cone(int i,vec3 toward_light) {
    if(forward_count==0) return 1.;
    vec4 parameters=texelFetch(forward_lights,i*4+3);
    if(parameters.y==0.) return 1.;
    vec4 direction=texelFetch(forward_lights,i*4+2);
    float cosine=dot(-toward_light,direction.xyz);
    if(parameters.x==direction.w) return float(cosine>=direction.w);
    return smoothstep(direction.w,parameters.x,cosine);
}
uint forward_word(int word) {
    int count=forward_total(),words=(count+31)/32;
#ifdef OPENRAC_PROBE_PIXEL
    ivec2 pixel=OPENRAC_PROBE_PIXEL;
#else
    ivec2 pixel=ivec2(gl_FragCoord.xy);
#endif
    pixel-=ivec2(forward_view.xy);
    ivec2 extent=ivec2(forward_view.zw);
    if(forward_tiled!=0 && all(greaterThanEqual(pixel,ivec2(0))) && all(lessThan(pixel,extent))) {
        ivec2 tile=pixel/16;
        int index=(tile.y*((extent.x+15)/16)+tile.x)*words*2+forward_transparent*words+word;
        return texelFetch(forward_masks,index).r;
    }
    int remaining=count-word*32;
    return remaining>=32?0xffffffffu:(1u<<uint(remaining))-1u;
}
int forward_next(inout uvec3 state) {
    if(forward_tiled==0) {int i=int(state.x++);return i<forward_total()?i:-1;}
    while(state.y==0u) {
        if(state.x>=uint((forward_total()+31)/32)) return -1;
        state.z=state.x++;
        state.y=forward_word(int(state.z));
    }
    int bit=findLSB(state.y);state.y&=state.y-1u;
    return int(state.z)*32+bit;
}
#define LOCAL_LIGHT_LOOP(i) uvec3 forward_state=uvec3(0);for(int i=forward_next(forward_state);i>=0;i=forward_next(forward_state))
#define LOCAL_POSITION(i) forward_position(i)
#define LOCAL_COLOUR(i) forward_colour(i)
#define LOCAL_CONE(i,d) forward_cone(i,d)
