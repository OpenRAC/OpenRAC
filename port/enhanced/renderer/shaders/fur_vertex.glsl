// SPDX-License-Identifier: GPL-3.0-or-later
uniform int hero_fur_debug, hero_fur_first, hero_fur_count, hero_fur_palette;
uniform int hero_fur_detail_enabled;
uniform int hero_fur_lighting_enabled;
layout(std430,binding=10) readonly buffer HeroFurVertices { uint hero_fur_vertices[]; };

int hero_fur_receiver(int vertex_id, float instance_palette, float instance_lighting) {
    int index = vertex_id - hero_fur_first;
    return int((hero_fur_debug != 0 || hero_fur_detail_enabled != 0 || hero_fur_lighting_enabled != 0) && hero_fur_palette >= 0 && instance_palette >= 0.
        && int(instance_palette + .5) == hero_fur_palette && instance_lighting < 1.5
        && index >= 0 && index < hero_fur_count && hero_fur_vertices[index] != 0u);
}
