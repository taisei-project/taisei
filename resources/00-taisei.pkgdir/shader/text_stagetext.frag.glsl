#version 330 core

#include "lib/render_context.glslh"
#include "lib/sprite_main.frag.glslh"
#include "lib/util.glslh"

float tc_mask(vec2 tc) {
    return float(tc.x >= 0 && tc.x <= 1 && tc.y >= 0 && tc.y <= 1);
}

void spriteMain(out vec4 fragColor) {
    float t = customParams.r;
    vec2 tc = flip_native_to_topleft(texCoord);
    vec2 tc_overlay = texCoordOverlay;

    tc *= dimensions;
    tc.x += sin(tc.y * 0.25 * (1 - pow(1 - t, 2))) * 2 * pow(1 - t, 1);
    tc.y += cos(tc.x * 0.25 * (1 - pow(1 - t, 2)) + pow(1 - t, 2) * pi * 2) * -2 * pow(1 - t, 0.5);
    tc /= dimensions;

    float a = tc_mask(tc);
    vec4 textfrag = color * texture(tex, uv_to_region(texRegion, flip_topleft_to_native(tc))).r * a;

    tc -= vec2(1) / dimensions;
    a = tc_mask(tc);

    vec4 shadowfrag = vec4(vec3(0), color.a) * texture(tex, uv_to_region(texRegion, flip_topleft_to_native(tc))).r * a;

    fragColor = textfrag;
    fragColor = mix(shadowfrag, textfrag, sqrt(textfrag.a));

    // The overlay coordinates are outside of [0,1] in the padding region, so we make sure there are no wrap around artifacts when a bit of text is distorted to this region.
	tc_overlay = clamp(tc_overlay, 0.01, 0.99);

    fragColor *= clamp((texture(tex_aux0, tc_overlay).r + 0.5) * 2.5 * t-0.5, 0.0, 1.0);
}
