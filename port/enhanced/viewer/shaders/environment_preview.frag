// SPDX-License-Identifier: GPL-3.0-or-later
// Appended to the shared environment atlas sampler. Display the sharpest retained reflection.
uniform vec3 preview_camera;
in vec3 preview_world;
in vec3 preview_normal;
flat in int preview_probe;
layout(location=0) out vec4 preview_colour;
void main() {
    if(preview_probe>=environment_count) discard;
    vec3 view=normalize(preview_camera-preview_world);
    vec3 direction=reflect(-view,normalize(preview_normal));
    vec3 colour=max(environment_level(preview_probe,direction,0,findMSB(environment_resolution)),vec3(0));
    // Same linear-to-display conversion as enhanced lighting, with no extra material or light.
    colour=mix(1.055*pow(colour,vec3(1./2.4))-.055,colour*12.92,lessThanEqual(colour,vec3(.0031308)));
    preview_colour=vec4(clamp(colour,0.,1.),1);
}
