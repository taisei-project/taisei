#version 330 core

#include "lib/render_context.glslh"
#include "interface/sprite.glslh"
#include "lib/util.glslh"

UNIFORM(1) sampler2D trans;

void main(void) {
    vec2 tc = texCoord;
    vec2 tc_overlay = texCoordOverlay;

    float t = customParam;

    vec2 f = tc_overlay-vec2(0.5,0.5);
    tc *= dimensions;
    tc += 10.*(f*sin(10.0*(length(f)+t)))*(1.0-t);
    tc /= dimensions;
    float a = float(tc.x >= 0 && tc.x <= 1 && tc.y >= 0 && tc.y <= 1);

    tc_overlay = clamp(tc_overlay,0.01,0.99);
    vec4 texel = texture(tex, uv_to_region(texRegion,tc));
    vec4 textfrag = vec4(color.rgb,a*color.a*texel.r);

    tc -= vec2(1.)/dimensions;
    a = float(tc.x >= 0 && tc.x <= 1 && tc.y >= 0 && tc.y <= 1);
    texel = texture(tex, uv_to_region(texRegion,tc));
    vec4 shadowfrag = vec4(0.,0.,0.,a*texel.r);

    fragColor = mix(shadowfrag,textfrag,sqrt(textfrag.a));
    fragColor.a *= clamp((texture(trans, tc_overlay).r+0.5)*2.5*t-0.5, 0.0, 1.0);
}
