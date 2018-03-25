#version 330 core

#include "render_context.glslh"
#include "uv_to_region.glslh"

uniform sampler2D tex;

in vec2 texCoord;
in vec2 texCoordRaw;
in vec4 color;
in float customParam;
in vec4 texRegion;

out vec4 fragColor;

float pi = 2.0 * asin(1.0);

vec2 g(vec2 x, float p) {
    return 0.5 * (pow(2.0 * x - 1.0, vec2(p)) + 1.0);
}

vec2 apply_deform(vec2 uv, float strength) {
    float p = 1.0 + strength * length(0.5 - uv);
    vec2 smaller = step(0.5 - uv, vec2(0.0));
    uv = smaller + (1.0 - 2.0 * smaller) * g(0.5 + abs(uv - 0.5), p);
    return 1.0 - uv;
}

void main(void) {
    vec2 uv = texCoordRaw;
    vec2 uv_orig = uv;
    vec4 texel;
    float deform = customParam;

    const float limit = 1.0;
    const float step = 0.25;
    const float num = 1.0 + limit / step;

    fragColor = vec4(0.0);

    for(float i = 0.0; i <= limit; i += step) {
        uv = apply_deform(uv_orig, deform * i);
        texel = texture2D(tex, uv_to_region(texRegion, uv));
        fragColor += vec4(color.rgb, color.a * texel.a);
    }

    fragColor /= num;
}
