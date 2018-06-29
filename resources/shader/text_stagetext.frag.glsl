#version 330 core

#include "lib/render_context.glslh"
#include "interface/sprite.glslh"
#include "lib/util.glslh"

UNIFORM(1) sampler2D trans;

float tc_mask(vec2 tc) {
	return float(tc.x >= 0 && tc.x <= 1 && tc.y >= 0 && tc.y <= 1);
}

void main(void) {
    float t = customParam;
    vec2 tc = texCoord;
    vec2 tc_overlay = texCoordOverlay;
    vec2 f = tc_overlay - vec2(0.5);

    tc *= dimensions;
    tc += 5 * f * sin(10 * (length(f) + t)) * (1 - t);
    tc /= dimensions;

    float a = tc_mask(tc);

    vec4 texel = texture(tex, uv_to_region(texRegion, tc));
    vec4 textfrag = vec4(color.rgb, a * color.a * texel.r);

    tc -= vec2(1) / dimensions;
    a = tc_mask(tc);

    texel = texture(tex, uv_to_region(texRegion, tc));
    vec4 shadowfrag = vec4(vec3(0), a * color.a * texel.r);

    fragColor = textfrag;
    fragColor = mix(shadowfrag, textfrag, sqrt(textfrag.a));
    fragColor.a *= clamp((texture(trans, tc_overlay).r + 0.5) * 2.5 * t-0.5, 0.0, 1.0);
}
