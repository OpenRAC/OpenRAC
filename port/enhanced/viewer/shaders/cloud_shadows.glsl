// SPDX-License-Identifier: GPL-3.0-or-later
#define OPENRAC_CLOUD_SHADOWS 1

uniform vec4 cloud_shadow_shape;  // Minimum light, maximum light, scale, strength.
uniform vec3 cloud_shadow_motion;  // XY drift and undulation phase.
uniform vec3 cloud_shadow_to_sun;
uniform sampler2D cloud_noise_texture;

float cloud_sun_attenuation = 1.0;

vec4 cloud_noise_taps(vec2 uv) {
    const float d = 1.0 / 192.0;  // One third of a 64x64 texel.
    return 0.25 * (textureLod(cloud_noise_texture, uv + vec2(-d, -d), 0.0)
                 + textureLod(cloud_noise_texture, uv + vec2(d, -d), 0.0)
                 + textureLod(cloud_noise_texture, uv + vec2(-d, d), 0.0)
                 + textureLod(cloud_noise_texture, uv + vec2(d, d), 0.0));
}

float cloud_noise(vec2 plane) {
    // The tile has eight noise cells per side. Apply the same scale to position and drift.
    vec2 uv = (plane + cloud_shadow_motion.xy) * (cloud_shadow_shape.z * 3.0);
    float time = cloud_shadow_motion.z;
    float offset = cloud_noise_taps(uv * .25 + time * vec2(.065, -.035)).r;
    float phase = time + offset * 2.0;
    int frame = int(floor(phase));
    int cycle = frame / 4;
    int channel = frame & 3;
    const vec2 jump = vec2(13.0, 29.0) / 64.0;
    vec4 current = cloud_noise_taps(uv + float(cycle) * jump);
    float next;
    if (channel == 3) {
        next = cloud_noise_taps(uv + float(cycle + 1) * jump).r;
    } else {
        next = current[channel + 1];
    }
    return mix(current[channel], next, smoothstep(0.0, 1.0, fract(phase)));
}

float cloud_contrast(float x) {
    float inverse = 1.0 - x;
    return x * x / (x * x + inverse * inverse);
}

float cloud_shadow_visibility(vec3 position, float sun_visibility) {
    // The cascade already accounts for fully sun-occluded receivers, including interiors.
    if (cloud_shadow_shape.w == 0.0 || sun_visibility <= 1.0 / 255.0) {
        return 1.0;
    }
    // World Z is up. Project onto Z=0 along the sun axis, preserving the slope near the horizon.
    float divisor = max(cloud_shadow_to_sun.z, .025);
    vec2 plane = (position.xy - cloud_shadow_to_sun.xy * (position.z / divisor))
                 * cloud_shadow_shape.z;
    float remapped = mix(cloud_shadow_shape.x, cloud_shadow_shape.y, cloud_noise(plane));
    return mix(1.0, cloud_contrast(cloud_contrast(remapped)), cloud_shadow_shape.w);
}

float clouded_sun_visibility(vec3 position, float sun_visibility) {
    return sun_visibility * cloud_shadow_visibility(position, sun_visibility);
}
