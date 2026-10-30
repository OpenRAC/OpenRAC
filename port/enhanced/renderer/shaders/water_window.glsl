// SPDX-License-Identifier: GPL-3.0-or-later
uniform int water_sim_local; // zero: full-domain verification fixtures
uniform int water_sim_body;
uniform float water_sim_height;
uniform float water_sim_weight;
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
vec4 water_sim_sample(sampler2D field,vec2 p,int body,vec2 legacy_uv,vec2 current) {
#ifndef OPENRAC_WATER_LOCAL
    if(water_sim_local==0) return textureLod(field,legacy_uv,0.);
#endif
    vec4 reference=vec4(0,current,water_sim_marker(p));
    float fade=water_sim_fade(p,body);
    if(fade<=0.) return reference;
    // World-addressed torus: moving the origin never shifts existing state.
    vec2 uv=fract(p/(water_sim_window.z*water_sim_window.w));
    return mix(reference,textureLod(field,uv,0.),fade);
}
vec4 water_sim_gradients(sampler2D field,vec2 p,int body,vec2 current,out float rotation) {
    float dx=water_sim_window.z;
    vec4 left=water_sim_sample(field,p-vec2(dx,0),body,vec2(0),current);
    vec4 right=water_sim_sample(field,p+vec2(dx,0),body,vec2(0),current);
    vec4 bottom=water_sim_sample(field,p-vec2(0,dx),body,vec2(0),current);
    vec4 top=water_sim_sample(field,p+vec2(0,dx),body,vec2(0),current);
    rotation=(right.z-left.z-top.y+bottom.y)/(2.*dx);
    return vec4(right.x-left.x,top.x-bottom.x,right.w-left.w,top.w-bottom.w)/(2.*dx);
}
