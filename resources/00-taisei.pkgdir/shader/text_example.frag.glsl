#version 330 core

#include "lib/render_context.glslh"
#include "lib/sprite_main.frag.glslh"
#include "lib/util.glslh"

void spriteMain(out vec4 fragColor) {
    vec2 tc = texCoord;
    vec2 tc_overlay = texCoordOverlay;

    // Transform local glyph texture coordinates.
    tc *= dimensions;
    tc.x += 5 * sin(3 * tc_overlay.y + customParams.r);
    tc /= dimensions;

    // Compute alpha multiplier used to chop off unwanted texels from the atlas.
    float a = float(tc.x >= 0 && tc.x <= 1 && tc.y >= 0 && tc.y <= 1);

    // Map local coordinates to the region of the atlas texture that contains our glyph.
    vec2 tc_atlas = uv_to_region(texRegion, tc);

    // Display the glyph.
    fragColor = color * vec4(texture(tex, tc_atlas).r * a);

    // Visualize global overlay coordinates. You could use them to span a texture across all glyphs.
    fragColor *= vec4(tc_overlay.x, tc_overlay.y, 0, 1);

    // Visualize the local coordinates.
    // fragColor = vec4(mod(tc.x, 1), float(length(tc - vec2(0.5, 0.5)) < 0.5), mod(tc.y, 1), 1.0);
}
