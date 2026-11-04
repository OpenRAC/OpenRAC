// SPDX-License-Identifier: GPL-3.0-or-later
// Screen-space transmission reads a separate pre-water scene, never the active attachment.
#define OPENRAC_EFFECT_COMPOSITE 1
uniform sampler2D water_scene_colour;
uniform sampler2D water_scene_depth;
uniform int water_scene_ready;
uniform vec2 water_scene_extent;
uniform mat4 water_scene_view;
// Projection Z coefficients: clip.z = x*view.z+y, clip.w = z*view.z+w.
uniform vec4 water_scene_projection;
bool water_composite_active=false;
vec3 water_background=vec3(0),water_background_weight=vec3(0),water_radiance=vec3(0);
float water_contact_coverage=1.;
vec3 water_contact_background=vec3(0);

// A narrow world-space contact band; optical detail must not move the shoreline.
float water_contact_blend(float normal_depth) {
    return smoothstep(0.,.12,normal_depth);
}

float water_scene_z(float depth) {
    float ndc=depth*2.-1.;
    return (water_scene_projection.y-ndc*water_scene_projection.w)
           /(ndc*water_scene_projection.z-water_scene_projection.x);
}
bool water_scene_behind(float depth,float surface_depth) {
    // Zero is cleared reversed depth (sky), not a submerged floor.
    return depth>0. && depth<=surface_depth;
}
// Returns normal thickness; water_transmittance applies Snell's refracted path length.
bool water_scene_sample(vec2 uv,float surface_depth,vec3 view_position,vec2 normal_offset,float nv,
                        float contact_nv,bool contact_blend,out vec3 background,out float thickness) {
    background=vec3(0);thickness=0.;
    water_contact_coverage=1.;
    water_contact_background=vec3(0);
    if(water_scene_ready==0) return false;
    ivec2 extent=ivec2(water_scene_extent);
    ivec2 pixel=clamp(ivec2(uv*water_scene_extent),ivec2(0),extent-1);
    float depth=texelFetch(water_scene_depth,pixel,0).r;
    if(!water_scene_behind(depth,surface_depth)) return false;
    float surface_z=water_scene_z(surface_depth);
    float ray_cosine=water_scene_projection.w!=0.?1.:abs(view_position.z)/max(length(view_position),1e-5);
    float path=max(0.,surface_z-water_scene_z(depth))/max(ray_cosine,1e-4);
    // Contact uses the unshifted depth and base surface normal, never the refracted floor.
    if(contact_blend) water_contact_coverage=water_contact_blend(path*clamp(contact_nv,0.,1.));
    // Six pixels maximum at the render resolution, fading smoothly to zero at contacts.
    vec2 offset=normal_offset*min(6.,path*2.);
    offset*=min(1.,6./max(length(offset),1e-5));
    offset*=water_contact_coverage;
    vec2 refracted=uv+offset/water_scene_extent;
    vec2 texel=refracted*water_scene_extent-.5;
    ivec2 base=ivec2(floor(texel));
    bool safe=all(greaterThanEqual(base,ivec2(0))) && all(lessThan(base+1,extent));
    // Check the whole bilinear footprint, preventing foreground/sky colour leaking at edges.
    if(safe) {
        for(int y=0;y<2;++y) for(int x=0;x<2;++x) {
            safe=safe && water_scene_behind(texelFetch(water_scene_depth,base+ivec2(x,y),0).r,surface_depth);
        }
    }
    // Only contacts need the unshifted scene colour in addition to the refracted sample.
    // The rejection path shares that exact texel and cannot blur foreground edges.
    if(water_contact_coverage<1. || !safe) {
        water_contact_background=probe_linear(texelFetch(water_scene_colour,pixel,0).rgb);
    }
    if(safe) {
        depth=texture(water_scene_depth,refracted).r;
        background=probe_linear(texture(water_scene_colour,refracted).rgb);
    } else {
        background=water_contact_background;
    }
    path=max(0.,surface_z-water_scene_z(depth))/max(ray_cosine,1e-4);
    thickness=path*clamp(nv,0.,1.);
    return true;
}
// Atmosphere applies only to new water radiance. Captured scene colour already contains fog.
bool effect_has_composite() { return water_composite_active; }
vec4 effect_composite(vec3 medium_transmittance,vec3 medium_scatter) {
    vec3 linear=water_radiance*medium_transmittance
                +probe_linear(medium_scatter)*(1.-water_background_weight)
                +water_background*water_background_weight;
    linear=mix(water_contact_background,linear,water_contact_coverage);
    return vec4(probe_display(max(linear,vec3(0))),1.);
}
