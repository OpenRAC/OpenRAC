// SPDX-License-Identifier: GPL-3.0-or-later
// Authored chrome/glass packets use GGX direct highlights and box-projected environment light.
vec3 chrome_direct(vec3 p,vec3 n,vec3 eye,vec4 selector,float visibility) {
    int a=(int(selector.x+.5)&15)*4,b=(int(selector.y+.5)&15)*4;
    float blend=clamp(selector.z,0.,1.);
    vec3 result=vec3(0);
    for(int light=0;light<2;++light) {
        int offset=light*2;
        vec3 radiance=max(mix(light_sets[a+offset].rgb,light_sets[b+offset].rgb,blend),vec3(0));
        vec3 travel=probe_unit(mix(light_sets[a+offset+1].xyz,light_sets[b+offset+1].xyz,blend));
        if(light==0) {
            if(probe_sunlight_direction.w!=0.) travel=probe_sunlight_direction.xyz;
            radiance*=probe_sunlight_scale;
        }
        result+=radiance*mix(.1,1.,visibility)*ggx_specular(n,eye,-travel,
                    object_reflection_material.x,object_reflection_material.y);
    }
    LOCAL_LIGHT_LOOP(i) {
        vec3 delta=LOCAL_POSITION(i).xyz-p;
        float radius=LOCAL_POSITION(i).w,distance=length(delta);
        if(radius<=0. || distance>=radius || distance<1e-6) continue;
        vec3 radiance=max(LOCAL_COLOUR(i).rgb,vec3(0))*LOCAL_ATTENUATION(distance,radius)*LOCAL_CONE(i,delta/distance);
        result+=radiance*ggx_specular(n,eye,delta/distance,
                    object_reflection_material.x,object_reflection_material.y);
    }
    return result;
}

vec4 chrome_surface(vec3 original) {
    if(object_reflection_enabled==0 || probe_mode!=1
        || probe_receiver!=2 || v_probe_selector.w>=1.5) return vec4(original,1);
    vec3 eye_position=(view*vec4(v_world_position,1)).xyz;
    vec3 geometric=cross(dFdx(eye_position),dFdy(eye_position));
    vec3 normal_view=orient_shading_normal(mat3(view)*v_normal,geometric,-eye_position);
    vec3 n=transpose(mat3(view))*normal_view;
    vec3 eye=probe_unit(transpose(mat3(view))*(-eye_position));
    float sun_visibility=1.,hero_visibility=1.;
    vec3 indirect=vec3(0);bool covered=false;
#ifdef OPENRAC_EARLY_LIGHTING
    vec4 cached;
    // Match diffuse meshes: reuse the early pass's resolved shadows rather than filtering again.
    early_surface(eye_position,normal_view,cached,covered,hero_visibility);
    sun_visibility=cached.a;
    indirect=cached.rgb;
#else
    sun_visibility=sun_shadow_visibility(v_world_position,n,n);
    hero_visibility=hero_shadow_visibility(v_world_position,n);
    covered=sample_probe_irradiance(v_world_position,n,probe_surface_bias(n,eye),indirect);
#endif
    if(!hero_shadow_applies(n)) hero_visibility=1.;
    float visibility=min(sun_visibility,hero_visibility);
    // Reuse early GI, including its directional normal response and local occlusion.
    // Original chrome is reserved for receivers without either source of indirect lighting.
    if(environment_count==0 && !covered) return vec4(original,1);
    vec3 fallback=covered ? gi_specular_fallback(indirect*probe_gi_multiplier,
                            dot(n,eye),object_reflection_material.y) : probe_linear(original);
    vec3 reflection=environment_specular(v_world_position,n,eye,object_reflection_material.x,
                        object_reflection_material.y,fallback);
    reflection+=chrome_direct(v_world_position,n,eye,v_probe_selector,visibility);
    reflection*=object_reflection_material.z*object_reflection_tint.rgb;
    return vec4(probe_display(reflection),object_reflection_tint.a);
}
