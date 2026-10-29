// SPDX-License-Identifier: GPL-3.0-or-later
// Rigid attachment pose in world space; keep wave slopes and transforms in FP32.
uniform int water_float_enabled;
mat4 water_float_pose(vec3 anchor) {
    if(water_float_enabled==0) return mat4(1);
    bool matched;
    vec3 centre=water_displaced_position(anchor,0,matched);
    if(!matched) return mat4(1);
    // Sample a half-unit footprint. At a bank, use the matched centre for a
    // one-sided tangent rather than taking a discontinuity from a dry cell.
    bool valid;
    vec3 left=water_displaced_position(anchor-vec3(.25,0,0),0,valid);
    if(!valid) left=centre;
    vec3 right=water_displaced_position(anchor+vec3(.25,0,0),0,valid);
    if(!valid) right=centre;
    vec3 bottom=water_displaced_position(anchor-vec3(0,.25,0),0,valid);
    if(!valid) bottom=centre;
    vec3 top=water_displaced_position(anchor+vec3(0,.25,0),0,valid);
    if(!valid) top=centre;
    vec3 up=cross(right-left,top-bottom);
    mat3 rotation=mat3(1);
    if(dot(up,up)>1e-12 && up.z>0.) {
        up=normalize(up);
        // Shortest rotation from world Z to the surface normal: no added spin.
        mat3 skew=mat3(0,0,-up.x, 0,0,-up.y, up.x,up.y,0);
        rotation+=skew+(skew*skew)/(1.+up.z);
    }
    // Ignore horizontal swell drift and preserve the authored anchor offset.
    vec3 offset=anchor+vec3(0,0,centre.z-anchor.z)-rotation*anchor;
    return mat4(vec4(rotation[0],0),vec4(rotation[1],0),vec4(rotation[2],0),vec4(offset,1));
}
