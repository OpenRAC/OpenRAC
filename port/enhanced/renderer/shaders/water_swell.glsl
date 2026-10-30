// SPDX-License-Identifier: GPL-3.0-or-later
// A low-steepness Gerstner swell layered over the interactive shallow water.
// Shared by vertex displacement and fragment normals so crest lighting agrees.
vec3 water_swell(vec2 p,float time,float weight,vec2 weight_gradient,out vec3 dx,out vec3 dy) {
    vec2 direction=normalize(vec2(.87,.49));
    float k=6.28318531/2.75;
    float phase=k*dot(direction,p)-time*1.4;
    float s=sin(phase),c=cos(phase);
    vec3 displacement=vec3(direction*(.045*.65*c),.045*s);
    vec3 derivative=vec3(-direction*(.045*.65*s),.045*c)*k;
    dx=weight*derivative*direction.x+displacement*weight_gradient.x;
    dy=weight*derivative*direction.y+displacement*weight_gradient.y;
    return weight*displacement;
}
vec3 water_art_tint(vec3 colour,vec3 painted_linear) {
    float luminance=max(dot(painted_linear,vec3(.2126,.7152,.0722)),.04);
    vec3 chroma=clamp(painted_linear/luminance,vec3(.4),vec3(1.8));
    return colour*mix(vec3(1),chroma,.32);
}

// Distance increases away from the nearest bank, so decreasing phase with time
// sends the crest inland. Fade where opposite banks meet and direction is weak.
vec3 water_boundary_swell(vec3 boundary,float time,float weight) {
    float magnitude=length(boundary.yz);
    vec2 direction=boundary.yz/max(magnitude,.001);
    float envelope=weight*smoothstep(.15,.65,magnitude)*exp(-boundary.x*.12);
    float phase=(6.28318531/2.2)*boundary.x-time*2.;
    return envelope*vec3(direction*(.0091*cos(phase)),.042*sin(phase));
}

// Spatial chain rule, independent of raster quads and triangle interpolation.
vec3 water_boundary_tangent(vec3 boundary,vec3 change,float time,float weight,float weight_change) {
    float magnitude=length(boundary.yz),safe_magnitude=max(magnitude,.001);
    vec2 direction=boundary.yz/safe_magnitude;
    float magnitude_change=magnitude>0.?dot(boundary.yz,change.yz)/magnitude:0.;
    vec2 direction_change=change.yz/safe_magnitude;
    if(magnitude>.001) direction_change-=direction*magnitude_change/magnitude;
    float t=clamp((magnitude-.15)/.5,0.,1.);
    float fade=t*t*(3.-2.*t);
    float fade_change=6.*t*(1.-t)*2.*magnitude_change;
    float attenuation=exp(-boundary.x*.12);
    float envelope=weight*fade*attenuation;
    float envelope_change=attenuation*(weight_change*fade+weight*fade_change-weight*fade*.12*change.x);
    float phase=(6.28318531/2.2)*boundary.x-time*2.;
    float phase_change=(6.28318531/2.2)*change.x;
    float c=cos(phase),s=sin(phase);
    return envelope_change*vec3(direction*(.0091*c),.042*s)
          +envelope*vec3(.0091*(direction_change*c-direction*s*phase_change),.042*c*phase_change);
}
