// SPDX-License-Identifier: GPL-3.0-or-later
// Packed cube faces and GGX roughness levels, with smooth spatial blending of two captures.
uniform sampler2D environment_map;
uniform int environment_count;
uniform int environment_resolution;
uniform samplerBuffer environment_metadata;
uniform int environment_columns;
vec4 environment_probe(int i) { return texelFetch(environment_metadata,i); }
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
int environment_row(int mip) {
    int row=0;
    // The source capture is full resolution; storage omits its sharpest mip (asset version 5).
    for(int i=0;i<mip;++i) row+=(environment_resolution>>(i+1))+2;
    return row;
}
vec3 environment_level(int probe,vec3 direction,int mip,int levels) {
    vec3 f=environment_face(direction);
    int size=environment_resolution>>(mip+1);
    int columns=max(environment_columns,1);
    vec2 origin=vec2((probe%columns)*6*((environment_resolution>>1)+2),(probe/columns)*environment_row(levels));
    vec2 pixel=origin+vec2(f.z*float(size+2),float(environment_row(mip)))+1.+f.xy*float(size);
    return textureLod(environment_map,pixel/vec2(textureSize(environment_map,0)),0.).rgb;
}
vec2 environment_axis(int id) { vec2 a=vec2(environment_probe(id*5).w,environment_probe(id*5+1).w);return dot(a,a)>.5?a:vec2(1,0); }
vec3 environment_local(int id,vec3 p) { vec2 a=environment_axis(id);return vec3(dot(p.xy,a),dot(p.xy,vec2(-a.y,a.x)),p.z); }
vec3 environment_world(int id,vec3 p) { vec2 a=environment_axis(id);return vec3(a.x*p.x-a.y*p.y,a.y*p.x+a.x*p.y,p.z); }
vec3 environment_project(int probe,vec3 p,vec3 direction) {
    vec3 original=direction;
    p=environment_local(probe,p);direction=environment_local(probe,direction);
    vec3 lo=environment_probe(probe*5+3).xyz,hi=environment_probe(probe*5+4).xyz;
    if(any(lessThan(p,lo)) || any(greaterThan(p,hi))) return original;
    vec3 limits=vec3(1e20);
    for(int axis=0;axis<3;++axis) {
        if(abs(direction[axis])>1e-6) limits[axis]=((direction[axis]>0.?hi[axis]:lo[axis])-p[axis])/direction[axis];
    }
    float distance=min(limits.x,min(limits.y,limits.z));
    return environment_world(probe,p+direction*max(distance,0.))-environment_probe(probe*5).xyz;
}
vec3 environment_specular(vec3 p,vec3 n,vec3 view,float roughness,float f0,vec3 fallback) {
    if(environment_count==0) return fallback;
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
        vec3 centre=environment_probe(i*5).xyz;
        vec3 lo=environment_probe(i*5+1).xyz,hi=environment_probe(i*5+2).xyz;
        vec3 local=environment_local(i,p);
        if(any(lessThan(local,lo)) || any(greaterThan(local,hi))) continue;
        // World-space blend bands also cover receivers close to their true supporting floor.
        vec3 edge=min(local-lo,hi-local);
        float fade=smoothstep(0.,.75,min(edge.x,min(edge.y,edge.z)));
        float weight=fade/(1.+dot(p-centre,p-centre)/(24.*24.));
        coverage=max(coverage,fade);
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
        radiance+=mix(environment_level(best[slot],ray,low,levels),environment_level(best[slot],ray,high,levels),fract(lod))*weights[slot];
    }
    radiance/=weights.x+weights.y;
    vec2 coordinate=(vec2(clamp(dot(n,view),0.,1.),roughness)*63.+.5);
    coordinate.y+=float(((environment_count+max(environment_columns,1)-1)/max(environment_columns,1))*environment_row(levels));
    vec2 brdf=textureLod(environment_map,coordinate/vec2(textureSize(environment_map,0)),0.).rg;
    return mix(fallback,radiance*(f0*brdf.x+brdf.y),coverage);
}
