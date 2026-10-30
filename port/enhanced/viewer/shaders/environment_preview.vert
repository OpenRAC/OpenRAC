#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Mirror spheres use the actual capture positions, generated in one draw without a CPU mesh.
uniform mat4 view_projection;
uniform samplerBuffer environment_metadata;
out vec3 preview_world;
out vec3 preview_normal;
flat out int preview_probe;
const ivec2 corners[6] =
    ivec2[6](ivec2(0,0),ivec2(1,0),ivec2(1,1),ivec2(0,0),ivec2(1,1),ivec2(0,1));
void main() {
    const int longitude=24,latitude=16,vertices=longitude*latitude*6;
    preview_probe=gl_VertexID/vertices;
    int vertex=gl_VertexID%vertices,cell=vertex/6;
    ivec2 at=ivec2(cell%longitude,cell/longitude)+corners[vertex%6];
    float azimuth=float(at.x)*6.28318530718/float(longitude);
    float polar=float(at.y)*3.14159265359/float(latitude);
    preview_normal=vec3(sin(polar)*cos(azimuth),sin(polar)*sin(azimuth),cos(polar));
    preview_world=texelFetch(environment_metadata,preview_probe*5).xyz+preview_normal*.75;
    gl_Position=view_projection*vec4(preview_world,1);
}
