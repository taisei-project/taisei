#version 330 core

#include "lib/render_context.glslh"
#include "interface/sprite.glslh"
#include "lib/util.glslh"

float tc_mask(vec2 tc) {
    return float(tc.x >= 0 && tc.x <= 1 && tc.y >= 0 && tc.y <= 1);
}

void main(void) {
    float gradient = 0.8 + 0.2 * flip_native_to_bottomleft(texCoordOverlay.y);
    vec2 tc = flip_native_to_topleft(texCoord);
    float a = tc_mask(tc);
    vec4 textfrag = color * texture(tex, uv_to_region(texRegion, flip_topleft_to_native(tc))).r * a;
    textfrag.rgb *= gradient;
    tc -= vec2(1) / dimensions;
    a = tc_mask(tc);
    vec4 shadowfrag = vec4(vec3(0), texture(tex, uv_to_region(texRegion, flip_topleft_to_native(tc))).r * color.a * a);
    fragColor = mix(shadowfrag, textfrag, sqrt(textfrag.a));
}
