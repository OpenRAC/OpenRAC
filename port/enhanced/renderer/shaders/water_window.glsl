// SPDX-License-Identifier: GPL-3.0-or-later
uniform int water_sim_local; // zero: full-domain verification fixtures
uniform int water_sim_body;
uniform float water_sim_height;
uniform float water_sim_weight;
// No curvature estimate is available when a clamped axis has only one neighbour.
float water_shore_curvature(float lo,float centre,float hi,float lower_step,float upper_step) {
    if(min(lower_step,upper_step)<1e-5) return 0.;
    return 2.*((hi-centre)/upper_step-(centre-lo)/lower_step)/(lower_step+upper_step);
}
int water_sim_shared_body(int body,float height) {
    return water_sim_body>=0 && abs(height-water_sim_height)<=.5?water_sim_body:body;
}
uniform vec4 water_sim_window; // lower world XY, cell spacing, cell count per side
float water_sim_marker(vec2 p) {
    return .5+.25*sin(dot(p,vec2(1.7,.9)))+.25*sin(dot(p,vec2(-.8,2.1)));
}
float water_sim_fade(vec2 p,int body) {
    if(body!=water_sim_body || water_sim_body<0) return 0.;
    vec2 centre=water_sim_window.xy+vec2(water_sim_window.z*water_sim_window.w*.5);
    return water_sim_weight*(1.-smoothstep(water_sim_window.z*water_sim_window.w*.375,water_sim_window.z*water_sim_window.w*.46875,length(p-centre)));
}
// Catmull-Rom height reconstruction on the repeating simulation torus, in FP32.
// Merge the positive middle weights into bilinear taps: nine reads instead of sixteen.
float water_sim_bicubic_height(sampler2D field,vec2 uv) {
    vec2 size=vec2(textureSize(field,0));
    vec2 pixel=uv*size-.5,base=floor(pixel),f=pixel-base;
    vec2 f2=f*f,f3=f2*f;
    vec2 w0=-.5*f+f2-.5*f3;
    vec2 w1=vec2(1.)-2.5*f2+1.5*f3;
    vec2 w2=.5*f+2.*f2-1.5*f3;
    vec2 w3=-.5*f2+.5*f3;
    vec2 middle=w1+w2;
    vec2 a=(base-.5)/size,b=(base+.5+w2/middle)/size,c=(base+2.5)/size;
    return dot(vec3(w0.y,middle.y,w3.y),vec3(
        dot(vec3(w0.x,middle.x,w3.x),vec3(textureLod(field,vec2(a.x,a.y),0.).x,textureLod(field,vec2(b.x,a.y),0.).x,textureLod(field,vec2(c.x,a.y),0.).x)),
        dot(vec3(w0.x,middle.x,w3.x),vec3(textureLod(field,vec2(a.x,b.y),0.).x,textureLod(field,vec2(b.x,b.y),0.).x,textureLod(field,vec2(c.x,b.y),0.).x)),
        dot(vec3(w0.x,middle.x,w3.x),vec3(textureLod(field,vec2(a.x,c.y),0.).x,textureLod(field,vec2(b.x,c.y),0.).x,textureLod(field,vec2(c.x,c.y),0.).x))));
}
vec4 water_sim_sample(sampler2D field,vec2 p,int body,vec2 legacy_uv,vec2 current) {
#ifndef OPENRAC_WATER_LOCAL
    if(water_sim_local==0) return textureLod(field,legacy_uv,0.);
#endif
    vec4 reference=vec4(0,current,water_sim_marker(p));
    float fade=water_sim_fade(p,body);
    if(fade<=0.) return reference;
    // World-addressed torus: moving the origin never shifts existing state.
    vec2 uv=fract(p/(water_sim_window.z*water_sim_window.w));
    vec4 sample_value=textureLod(field,uv,0.);
    sample_value.x=water_sim_bicubic_height(field,uv);
    return mix(reference,sample_value,fade);
}
vec4 water_sim_gradients(sampler2D field,vec2 p,int body,vec2 current,float height,out float rotation,out float compression,out float curvature) {
    float dx=water_sim_window.z;
    vec4 left=water_sim_sample(field,p-vec2(dx,0),body,vec2(0),current);
    vec4 right=water_sim_sample(field,p+vec2(dx,0),body,vec2(0),current);
    vec4 bottom=water_sim_sample(field,p-vec2(0,dx),body,vec2(0),current);
    vec4 top=water_sim_sample(field,p+vec2(0,dx),body,vec2(0),current);
    rotation=(right.z-left.z-top.y+bottom.y)/(2.*dx);
    compression=-(right.y-left.y+top.z-bottom.z)/(2.*dx);
    curvature=(4.*height-left.x-right.x-bottom.x-top.x)/(dx*dx);
    return vec4(right.x-left.x,top.x-bottom.x,right.w-left.w,top.w-bottom.w)/(2.*dx);
}
