// SPDX-License-Identifier: GPL-3.0-or-later
uniform samplerBuffer atmosphere_volume;
uniform int atmosphere_enabled;
uniform vec2 atmosphere_grid_size;
#define atmosphere_grid ivec2(atmosphere_grid_size)
uniform vec4 atmosphere_screen;
uniform mat4 atmosphere_view;
uniform float atmosphere_far;
uniform int atmosphere_inscatter;
uniform int atmosphere_slices;
// Four records per cell: combined/fog-only endpoint transport and their source/extinction.
vec4 atmosphere_column(ivec2 xy,float depth,bool sky) {
    xy=clamp(xy,ivec2(0),atmosphere_grid-1);
    float d=clamp(depth,0.,atmosphere_far);
    float slice=log(1.+d)/log(1.+atmosphere_far)*float(atmosphere_slices);
    int z=clamp(int(floor(slice)),0,atmosphere_slices-1);
    int channel=sky?1:0;
    int index=(z*atmosphere_grid.y*atmosphere_grid.x+xy.y*atmosphere_grid.x+xy.x)*4;
    vec4 prefix=z==0?vec4(0,0,0,1):texelFetch(atmosphere_volume,index-atmosphere_grid.x*atmosphere_grid.y*4+channel);
    vec4 medium=texelFetch(atmosphere_volume,index+2+channel);
    float start=exp(log(1.+atmosphere_far)*float(z)/float(atmosphere_slices))-1.;
    return atmosphere_step(prefix,medium.rgb,medium.a,max(0.,d-start));
}
vec4 atmosphere_sample(float depth,bool sky) {
    vec2 p=(gl_FragCoord.xy-atmosphere_screen.xy)/atmosphere_screen.zw*vec2(atmosphere_grid)-.5;
    ivec2 cell=ivec2(floor(p));vec2 f=fract(p);
    return mix(mix(atmosphere_column(cell,depth,sky),atmosphere_column(cell+ivec2(1,0),depth,sky),f.x),
               mix(atmosphere_column(cell+ivec2(0,1),depth,sky),atmosphere_column(cell+ivec2(1,1),depth,sky),f.x),f.y);
}
vec3 atmosphere_apply(vec3 colour,vec3 world,bool inscatter) {
    if(atmosphere_enabled==0) return colour;
    float depth=-(atmosphere_view*vec4(world,1)).z;
    vec4 transport=atmosphere_sample(depth,false);
    return colour*transport.a+(inscatter?transport.rgb:vec3(0));
}

// Sky always consumes the final fog-only endpoint: no partial-segment integration.
vec4 atmosphere_sky_column(ivec2 xy) {
    xy=clamp(xy,ivec2(0),atmosphere_grid-1);
    int index=((atmosphere_slices-1)*atmosphere_grid.y*atmosphere_grid.x+xy.y*atmosphere_grid.x+xy.x)*4;
    return texelFetch(atmosphere_volume,index+1);
}
vec4 atmosphere_sky_sample() {
    vec2 p=(gl_FragCoord.xy-atmosphere_screen.xy)/atmosphere_screen.zw*vec2(atmosphere_grid)-.5;
    ivec2 cell=ivec2(floor(p));vec2 f=fract(p);
    return mix(mix(atmosphere_sky_column(cell),atmosphere_sky_column(cell+ivec2(1,0)),f.x),
               mix(atmosphere_sky_column(cell+ivec2(0,1)),atmosphere_sky_column(cell+ivec2(1,1)),f.x),f.y);
}
