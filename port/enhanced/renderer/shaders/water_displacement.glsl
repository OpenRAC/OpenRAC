// SPDX-License-Identifier: GPL-3.0-or-later
// Shared liquid/attachment displacement; coordinates and simulation values remain FP32.
uniform samplerBuffer water_bodies;
uniform sampler2D water_wave_map;
uniform sampler2D water_coast_map;
uniform float water_time;
uniform float water_current_scale;
uniform vec4 water_atlas[64];
uniform vec2 water_atlas_extent;
uniform int water_body_count;
vec3 water_displaced_position(vec3 p,int surface,out bool matched) {
    matched=false;
    for (int i=0;i<water_body_count;++i) {
        vec4 origin=texelFetch(water_bodies,i*5);
        if (surface!=0 && int(origin.w)!=surface) continue;
        vec4 u=texelFetch(water_bodies,i*5+1),v=texelFetch(water_bodies,i*5+2);
        vec4 normal=texelFetch(water_bodies,i*5+3);
        float dx=texelFetch(water_bodies,i*5+4).z;
        vec2 cell=vec2(dot(p-origin.xyz,u.xyz),dot(p-origin.xyz,v.xyz))/dx;
        if (any(lessThan(cell,vec2(0))) || any(greaterThanEqual(cell,vec2(u.w,v.w)))) continue;
        // Clamp within this body, never filter into the next atlas rectangle.
        vec2 uv=(water_atlas[i].xy+clamp(cell,vec2(.5),vec2(u.w,v.w)-.5))/water_atlas_extent;
        vec4 coast=textureLod(water_coast_map,uv,0.);
        // Atlas alpha carries rest Z; the grid origin itself is always at Z=0.
        // Match the nearby water column rather than a different storey or a dry bank.
        if(surface==0 && (coast.x<.5 || abs(p.z-coast.w)>1.)) continue;
        // Pin the swell to the bank and taper it out in shallow/dry cells.
        float swell_weight=clamp(coast.x,0.,1.)*smoothstep(0.,1.2,coast.y)
                           *smoothstep(.03,.4,coast.z);
        vec2 lo=clamp(cell-1.,vec2(.5),vec2(u.w,v.w)-.5);
        vec2 hi=clamp(cell+1.,vec2(.5),vec2(u.w,v.w)-.5);
        vec2 centre=clamp(cell,vec2(.5),vec2(u.w,v.w)-.5);
        float left=textureLod(water_coast_map,(water_atlas[i].xy+vec2(lo.x,centre.y))/water_atlas_extent,0.).y;
        float right=textureLod(water_coast_map,(water_atlas[i].xy+vec2(hi.x,centre.y))/water_atlas_extent,0.).y;
        float bottom=textureLod(water_coast_map,(water_atlas[i].xy+vec2(centre.x,lo.y))/water_atlas_extent,0.).y;
        float top=textureLod(water_coast_map,(water_atlas[i].xy+vec2(centre.x,hi.y))/water_atlas_extent,0.).y;
        vec2 distance_gradient=vec2(right-left,top-bottom)/(max(hi-lo,vec2(.001))*dx);
        vec3 boundary=vec3(coast.y,u.xy*distance_gradient.x+v.xy*distance_gradient.y);
        vec3 sx,sy;
        vec3 swell=water_swell(p.xy,water_time,swell_weight,vec2(0),sx,sy);
        matched=true;
        return p+normal.xyz*water_sim_sample(water_wave_map,p.xy,water_sim_shared_body(i,surface==0?coast.w:p.z),uv,texelFetch(water_bodies,i*5+4).xy*.8*water_current_scale).x+swell
               +water_boundary_swell(boundary,water_time,swell_weight);
    }
    return p;
}


vec3 water_displaced_position(vec3 p,int surface) {
    bool matched;
    return water_displaced_position(p,surface,matched);
}
