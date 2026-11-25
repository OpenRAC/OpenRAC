// SPDX-License-Identifier: GPL-3.0-or-later
// Packed cube faces and GGX roughness levels, with smooth spatial blending of two captures.
uniform sampler2D environment_map;
uniform sampler2D environment_brdf;
uniform int environment_count;
uniform int environment_resolution;
uniform int environment_alignment;
uniform samplerBuffer environment_metadata;
uniform int environment_columns;
vec4 environment_probe(int i) { return texelFetch(environment_metadata,i); }
// openrac_mode: environment_tiled
uniform int environment_tiled;
uniform usamplerBuffer environment_tiles;
uniform vec4 environment_tile_view; // Viewport origin and extent, in current render pixels.

int environment_tile() {
    if(environment_tiled==0) return -1;
    ivec2 pixel=ivec2(gl_FragCoord.xy)-ivec2(environment_tile_view.xy);
    ivec2 extent=ivec2(environment_tile_view.zw);
    if(any(lessThan(pixel,ivec2(0))) || any(greaterThanEqual(pixel,extent))) return -1;
    ivec2 tile=pixel/16;
    return (tile.y*((extent.x+15)/16)+tile.x)*17;
}
uint environment_candidate_words() {
    int tile=environment_tile();
    return tile<0 ? (1u<<uint((environment_count+31)/32))-1u : texelFetch(environment_tiles,tile).r;
}
uint environment_candidates(int word) {
    int tile=environment_tile();
    if(tile>=0) return texelFetch(environment_tiles,tile+1+word).r;
    int remaining=environment_count-word*32;
    return remaining<=0 ? 0u : remaining>=32 ? 0xffffffffu : (1u<<uint(remaining))-1u;
}

vec3 environment_face(vec3 d) {
    vec3 a=abs(d);vec2 uv;float face;
    if(a.x>=a.y && a.x>=a.z) {
        face=d.x>=0.?0.:1.;uv=vec2(d.x>=0.?-d.z:d.z,-d.y)/a.x;
    } else if(a.y>=a.z) {
        face=d.y>=0.?2.:3.;uv=vec2(d.x,d.y>=0.?d.z:-d.z)/a.y;
    } else {
        face=d.z>=0.?4.:5.;uv=vec2(d.z>=0.?d.x:-d.x,-d.y)/a.z;
    }
    return vec3(uv*.5+.5,face);
}
// Every face and GGX level starts on its compression block boundary.
int environment_pitch(int mip) { int a=max(environment_alignment,1); return (((environment_resolution>>(mip+1))+2+a-1)/a)*a; }
int environment_row(int mip) {
    int row=0;
    // The source capture is full resolution; storage omits its sharpest mip.
    for(int i=0;i<mip;++i) row+=environment_pitch(i);
    return row;
}
vec3 environment_level(int probe,vec3 direction,int mip,int levels) {
    vec3 f=environment_face(direction);
    int size=environment_resolution>>(mip+1);
    int columns=max(environment_columns,1);
    vec2 origin=vec2((probe%columns)*6*environment_pitch(0),(probe/columns)*environment_row(levels));
    vec2 pixel=origin+vec2(f.z*float(environment_pitch(mip)),float(environment_row(mip)))+1.+f.xy*float(size);
    return textureLod(environment_map,pixel/vec2(textureSize(environment_map,0)),0.).rgb;
}
vec3 environment_rotation_world(vec3 p,vec4 q) { if(dot(q,q)<.5)q=vec4(0,0,0,1);return p+2.*cross(q.xyz,cross(q.xyz,p)+q.w*p); }
vec3 environment_rotation_local(vec3 p,vec4 q) { return environment_rotation_world(p,vec4(-q.xyz,q.w)); }
vec4 environment_frame(int id) { return vec4(environment_probe(id*6).w,environment_probe(id*6+1).w,environment_probe(id*6+2).w,environment_probe(id*6+3).w); }
vec3 environment_local(int id,vec3 p) { return environment_rotation_local(p,environment_frame(id)); }
vec3 environment_world(int id,vec3 p) { return environment_rotation_world(p,environment_frame(id)); }
vec3 environment_projection_local(int id,vec3 p) { return environment_rotation_local(p,environment_probe(id*6+5)); }
vec3 environment_projection_world(int id,vec3 p) { return environment_rotation_world(p,environment_probe(id*6+5)); }
int environment_capture(int region) { return int(environment_probe(region*6+4).w+.5); }
vec3 environment_project(int probe,vec3 p,vec3 direction) {
    vec3 original=direction;
    p=environment_projection_local(probe,p);direction=environment_projection_local(probe,direction);
    vec3 lo=environment_probe(probe*6+3).xyz,hi=environment_probe(probe*6+4).xyz;
    if(any(lessThan(p,lo)) || any(greaterThan(p,hi))) return original;
    vec3 limits=vec3(1e20);
    for(int axis=0;axis<3;++axis) {
        if(abs(direction[axis])>1e-6) limits[axis]=((direction[axis]>0.?hi[axis]:lo[axis])-p[axis])/direction[axis];
    }
    float distance=min(limits.x,min(limits.y,limits.z));
    return environment_projection_world(probe,p+direction*max(distance,0.))-environment_probe(probe*6).xyz;
}
// A bounded receiver offset selects the reflection medium at an interface. Parallax stays at p.
// Keep the bound in sync with the conservative expansion in environment_selection.comp.
// Separate lookup orientation from the shading cosine for anisotropic reflection approximations.
vec3 environment_specular_surface_frame(vec3 p,vec3 receiver,vec3 geometric_normal,vec3 n,vec3 view,float nv,float roughness,vec3 f0,vec3 fallback) {
    vec3 offset=receiver-p;
    receiver=p+offset*min(1.,.5/max(length(offset),1e-6));
    if(environment_count==0) return fallback;
    vec3 facing_normal=geometric_normal*inversesqrt(max(dot(geometric_normal,geometric_normal),1e-12));
    ivec2 best=ivec2(-1);vec2 weights=vec2(0);float coverage=0.;
    uint words=environment_candidate_words();
    // Ascending IDs preserve the full-loop tie order and floating-point blending result.
    while(words!=0u) {
      int word=findLSB(words);words&=words-1u;
      uint candidates=environment_candidates(word);
      while(candidates!=0u) {
        int i=word*32+findLSB(candidates);
        candidates&=candidates-1u;
        if(i>=environment_count) continue;
        vec3 centre=environment_probe(i*6).xyz;
        vec3 lo=environment_probe(i*6+1).xyz,hi=environment_probe(i*6+2).xyz;
        vec3 local=environment_local(i,receiver);
        if(any(lessThan(local,lo)) || any(greaterThan(local,hi))) continue;
        // World-space blend bands also cover receivers close to their true supporting floor.
        vec3 edge=min(local-lo,hi-local);
        float fade=smoothstep(0.,.75,min(edge.x,min(edge.y,edge.z)));
        vec3 to_probe=centre-p;
        float distance_squared=dot(to_probe,to_probe);
        // Wrapped facing preference, with residual support behind the surface.
        // The capture is a viewpoint, not a light: normalize the blend below and
        // leave spatial coverage independent so a sole probe never loses energy.
        float wrap=clamp(.5*(1.+dot(facing_normal,to_probe)*inversesqrt(max(distance_squared,1e-12))),0.,1.);
        float weight=fade*(wrap*wrap+.2)/(1.+distance_squared/(24.*24.));
        coverage=max(coverage,fade);
        // Overlapping regions of one photograph contribute their maximum weight,
        // never two blend slots or an artificially doubled contribution.
        if(best.x>=0 && environment_capture(best.x)==environment_capture(i)) {
            if(weight>weights.x) {weights.x=weight;best.x=i;}
            continue;
        }
        if(best.y>=0 && environment_capture(best.y)==environment_capture(i)) {
            if(weight>weights.y) {weights.y=weight;best.y=i;}
            if(weights.y>weights.x) {weights=weights.yx;best=best.yx;}
            continue;
        }
        if(weight>weights.x) {weights.y=weights.x;best.y=best.x;weights.x=weight;best.x=i;}
        else if(weight>weights.y) {weights.y=weight;best.y=i;}
    }
    }
    if(best.x<0 || weights.x+weights.y<1e-8) return fallback;
    int levels=findMSB(environment_resolution);
    // Clamp both radiance and BRDF to the first retained roughness; exact mirrors are unavailable.
    roughness=clamp(roughness,1./float(levels),1.);
    float lod=clamp(roughness*float(levels)-1.,0.,float(levels-1));
    int low=int(floor(lod)),high=min(low+1,levels-1);
    vec3 direction=reflect(-view,n),radiance=vec3(0);
    for(int slot=0;slot<2;++slot) {
        if(best[slot]<0) continue;
        vec3 ray=environment_project(best[slot],p,direction);
        radiance+=mix(environment_level(environment_capture(best[slot]),ray,low,levels),environment_level(environment_capture(best[slot]),ray,high,levels),fract(lod))*weights[slot];
    }
    radiance/=weights.x+weights.y;
    vec2 coordinate=(vec2(clamp(nv,0.,1.),roughness)*63.+.5);
    vec2 brdf=textureLod(environment_brdf,coordinate/64.,0.).rg;
    return mix(fallback,radiance*(f0*brdf.x+brdf.y),coverage);
}

// Callers without a separate geometric normal use their surface normal.
vec3 environment_specular_surface_frame(vec3 p,vec3 receiver,vec3 n,vec3 view,float nv,float roughness,vec3 f0,vec3 fallback) {
    return environment_specular_surface_frame(p,receiver,n,n,view,nv,roughness,f0,fallback);
}

// MoltenVK translates to Metal's shading language, which implicitly converts floats to vectors.
// Separate names avoid an ambiguous float/vec3 overload.
vec3 environment_specular_surface_frame_scalar(vec3 p,vec3 receiver,vec3 n,vec3 view,float nv,float roughness,float f0,vec3 fallback) {
    return environment_specular_surface_frame(p,receiver,n,view,nv,roughness,vec3(f0),fallback);
}

vec3 environment_specular_surface(vec3 p,vec3 receiver,vec3 n,vec3 view,float roughness,float f0,vec3 fallback) {
    return environment_specular_surface_frame_scalar(p,receiver,n,view,dot(n,view),roughness,f0,fallback);
}

vec3 environment_specular_frame(vec3 p,vec3 n,vec3 view,float nv,float roughness,float f0,vec3 fallback) {
    return environment_specular_surface_frame_scalar(p,p,n,view,nv,roughness,f0,fallback);
}

// Callers without an explicit surface receiver retain their original selection point.
vec3 environment_specular(vec3 p,vec3 n,vec3 view,float roughness,float f0,vec3 fallback) {
    return environment_specular_surface(p,p,n,view,roughness,f0,fallback);
}

vec3 environment_specular_rgb(vec3 p,vec3 n,vec3 view,float roughness,vec3 f0,vec3 fallback) {
    return environment_specular_surface_frame(p,p,n,view,dot(n,view),roughness,f0,fallback);
}
