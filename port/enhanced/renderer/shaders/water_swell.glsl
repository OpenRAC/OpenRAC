// SPDX-License-Identifier: GPL-3.0-or-later
// A low-steepness Gerstner swell layered over the interactive shallow water.
// Shared by vertex displacement and fragment normals so crest lighting agrees.
#ifdef OPENRAC_OCEAN
uniform sampler2D water_ocean_field;
#ifdef OPENRAC_OCEAN_SPECTRUM
uniform sampler2D water_ocean_detail;
uniform sampler2D water_ocean_displacement;
uniform vec2 water_ocean_tiles;
uniform vec3 water_ocean_shape; // Horizontal strength and the two bands' slope variances.
uniform vec4 water_ocean_view; // Camera XY, near radius, and horizon radius.
uniform vec4 water_ocean_tuning; // Chop, ripples, cosine and sine of the wind offset.
#ifdef OPENRAC_OCEAN_FINE
uniform sampler2D water_ocean_fine;
float water_ocean_footprint;
float water_ocean_unresolved_variance=0.;
mat2 water_ocean_horizontal_gradient=mat2(0.);
#endif
#endif
uniform vec4 water_ocean_grid;
uniform vec4 water_ocean_modes[8];
uniform vec4 water_ocean_amplitudes[2];
uniform int water_ocean_mode_count;
uniform int water_ocean_coastal;

// The last row is the inverse FFT of one bin. Rotating its lookup aligns it with the bake.
vec2 ocean_basis(vec2 local, int side, vec4 mode) {
    float sample_x = fract(dot(local, mode.xy) * mode.z / 6.28318531)
        * float(side);
    int x = int(sample_x);
    return mix(texelFetch(water_ocean_field, ivec2(x, side), 0).xy,
               texelFetch(water_ocean_field, ivec2((x+1)%side, side), 0).xy,
               fract(sample_x));
}

float ocean_height(vec2 p, vec2 rotations[2]) {
    vec2 local = p - water_ocean_grid.xy;
    int side = int(water_ocean_grid.w);
    vec4 correction = vec4(0.);
    vec2 cell = local / water_ocean_grid.z;
#ifdef OPENRAC_OCEAN_SPECTRUM
    // Keep the incident waves phase-matched across the bake. Beyond it, blend
    // them out so two perfectly straight wave trains cannot span the ocean.
    vec2 outside = max(max(-cell, cell - float(side)), vec2(0.));
    float incident_weight = 1. - smoothstep(0., 12., length(outside));
    if (incident_weight == 0.) return 0.;
#else
    float incident_weight = 1.;
#endif
    if (water_ocean_coastal != 0 && all(greaterThanEqual(cell, vec2(0.)))
        && all(lessThan(cell, vec2(side)))) {
        vec2 uv = clamp(cell, vec2(.5), vec2(side)-.5) / vec2(side, side+1);
        correction = textureLod(water_ocean_field, uv, 0.);
    }
    float height = 0.;
    for (int i = 0; i < min(water_ocean_mode_count, 2); ++i) {
        vec2 basis = incident_weight * ocean_basis(local, side, water_ocean_modes[i]);
        if (i == 0) basis += correction.rg;
        if (i == 1) basis += correction.ba;
        height += dot(basis, rotations[i]);
    }
    return height;
}

vec3 water_swell(vec2 p,float time,float weight,vec2 weight_gradient,out vec3 dx,out vec3 dy) {
    vec2 cell = (p-water_ocean_grid.xy) / water_ocean_grid.z;
    vec2 edge = min(cell, water_ocean_grid.w-cell);
    float t = clamp(min(edge.x,edge.y)/8.,0.,1.);
    float fade = t*t*(3.-2.*t);
    vec2 edge_gradient = edge.x < edge.y
        ? vec2(sign(water_ocean_grid.w*.5-cell.x),0.)
        : vec2(0.,sign(water_ocean_grid.w*.5-cell.y));
    weight_gradient = fade*weight_gradient
        + (weight-1.)*(6.*t*(1.-t)/(8.*water_ocean_grid.z))*edge_gradient;
    weight = mix(1.,weight,fade);
    vec2 rotations[2];
    for (int i = 0; i < min(water_ocean_mode_count, 2); ++i) {
        float phase = time * water_ocean_modes[i].w + 1.7 * float(i);
        rotations[i] = water_ocean_amplitudes[0][i] * vec2(cos(phase), sin(phase));
    }
    float height = ocean_height(p, rotations);
    float step_size = water_ocean_grid.z;
    vec2 gradient = vec2(
        ocean_height(p + vec2(step_size,0.), rotations) - ocean_height(p - vec2(step_size,0.), rotations),
        ocean_height(p + vec2(0.,step_size), rotations) - ocean_height(p - vec2(0.,step_size), rotations)
    ) / (2. * step_size);
    vec2 local = p - water_ocean_grid.xy;
#ifdef OPENRAC_OCEAN_SPECTRUM
    // The FFT tile wraps in both axes, independently of the clamped coastal field.
    vec2 wind = water_ocean_tuning.zw;
    mat2 wind_rotation = mat2(wind.x, -wind.y, wind.y, wind.x);
    vec2 wind_local = wind_rotation * local;
    vec2 uv = wind_local / water_ocean_tiles.x + .5 / water_ocean_grid.w;
    vec4 detail = textureLod(water_ocean_detail, uv, 0.);
    // A second orientation breaks up the repeating tile offshore.
    // Transform its slopes back to world axes before combining the fields.
    const mat2 turn = mat2(.809017, .587785, -.587785, .809017);
    vec2 rotated_uv = turn * wind_local / water_ocean_tiles.x + vec2(.31, .63);
    vec4 rotated = textureLod(water_ocean_detail, rotated_uv, 0.);
    vec4 displacement = textureLod(water_ocean_displacement, uv, 0.);
    vec4 rotated_displacement = textureLod(water_ocean_displacement, rotated_uv, 0.);
    float choppiness = .707107 * water_ocean_shape.x * water_ocean_tuning.x;
    vec2 horizontal = transpose(wind_rotation)
        * (displacement.xy + transpose(turn) * rotated_displacement.xy) * choppiness;
    float horizontal_length = length(horizontal);
    float horizontal_clamp = min(1., .7/max(horizontal_length,1e-5));
    horizontal *= horizontal_clamp;
    height += .707107 * (detail.x + rotated.x) * water_ocean_tuning.x;
    float broad_weight = 1.;
#ifdef OPENRAC_OCEAN_FINE
    // Both derivatives come from the same filtered FFT grids as displacement.
    mat2 jacobian = mat2(displacement.z,detail.w,detail.w,displacement.w);
    mat2 rotated_jacobian = mat2(rotated_displacement.z,rotated.w,rotated.w,rotated_displacement.w);
    jacobian = transpose(wind_rotation) * (jacobian + transpose(turn)*rotated_jacobian*turn)
        * wind_rotation * choppiness;
    if(horizontal_clamp<1.) {
        vec2 direction=horizontal/.7;
        jacobian=(mat2(1.)-outerProduct(direction,direction))*jacobian*horizontal_clamp;
    }
    vec2 fine_uv = wind_local / water_ocean_tiles.y + .5 / water_ocean_grid.w;
    float footprint = water_ocean_footprint;
    broad_weight = 1. - smoothstep(1.2,5.,footprint);
    float fine_weight = 1. - smoothstep(.16,.64,footprint);
    vec2 fine_slope = textureLod(water_ocean_fine, fine_uv, 0.).yz;
    gradient += transpose(wind_rotation) * fine_slope * (fine_weight * water_ocean_tuning.y);
    vec2 resolved = vec2(broad_weight,fine_weight);
    vec2 variance = water_ocean_shape.yz * water_ocean_tuning.xy * water_ocean_tuning.xy;
    water_ocean_unresolved_variance = dot(variance,1.-resolved*resolved)*weight*weight;
#endif
    gradient += transpose(wind_rotation) * (detail.yz + transpose(turn)*rotated.yz)
        * (.707107 * broad_weight * water_ocean_tuning.x);
#else
    // Legacy assets store their short waves as uncorrected single bins.
    for (int i = 2; i < water_ocean_mode_count; ++i) {
        vec4 mode = water_ocean_modes[i];
        vec2 wave_vector = mode.xy * mode.z;
        float phase = dot(local, wave_vector) - time * mode.w - 1.7 * float(i);
        float amplitude = water_ocean_amplitudes[i / 4][i % 4];
        height += amplitude * cos(phase);
        gradient -= (amplitude * sin(phase) / step_size) * sin(wave_vector * step_size);
    }
#endif
    vec2 slope = weight * gradient + height * weight_gradient;
#ifdef OPENRAC_OCEAN_SPECTRUM
    float distance_to_eye = length(p-water_ocean_view.xy);
    float displacement_fade = 1. - smoothstep(2.*water_ocean_view.z, 6.*water_ocean_view.z, distance_to_eye);
#ifdef OPENRAC_OCEAN_FINE
    float fade_range=4.*water_ocean_view.z;
    float fade_t=clamp((distance_to_eye-2.*water_ocean_view.z)/fade_range,0.,1.);
    vec2 fade_gradient=-(6.*fade_t*(1.-fade_t)/fade_range)
        *(p-water_ocean_view.xy)/max(distance_to_eye,1e-5);
    water_ocean_horizontal_gradient = jacobian*(weight*displacement_fade)
        + outerProduct(horizontal,weight_gradient*displacement_fade+weight*fade_gradient);
#endif
    height *= displacement_fade;
    horizontal *= displacement_fade;
    slope *= 1. - smoothstep(.1*water_ocean_view.w, .6*water_ocean_view.w, distance_to_eye);
#endif
    dx = vec3(0.,0.,slope.x);
    dy = vec3(0.,0.,slope.y);
#ifdef OPENRAC_OCEAN_SPECTRUM
    return weight * vec3(horizontal,height);
#else
    return vec3(0.,0.,weight * height);
#endif
}
#else
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
#endif
vec3 water_art_tint(vec3 colour,vec3 painted_linear) {
    float luminance=max(dot(painted_linear,vec3(.2126,.7152,.0722)),.04);
    vec3 chroma=clamp(painted_linear/luminance,vec3(.4),vec3(1.8));
    return colour*mix(vec3(1),chroma,.32);
}

// Distance increases away from the nearest bank, so decreasing phase with time
// sends the crest inland. Fade where opposite banks meet and direction is weak.
vec3 water_boundary_swell(vec3 boundary,float time,float weight) {
#ifdef OPENRAC_OCEAN
    return vec3(0.);
#else
    float magnitude=length(boundary.yz);
    vec2 direction=boundary.yz/max(magnitude,.001);
    float envelope=weight*smoothstep(.15,.65,magnitude)*exp(-boundary.x*.12);
    float phase=(6.28318531/2.2)*boundary.x-time*2.;
    return envelope*vec3(direction*(.0091*cos(phase)),.042*sin(phase));
#endif
}

// Spatial chain rule, independent of raster quads and triangle interpolation.
vec3 water_boundary_tangent(vec3 boundary,vec3 change,float time,float weight,float weight_change) {
#ifdef OPENRAC_OCEAN
    return vec3(0.);
#else
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
#endif
}
