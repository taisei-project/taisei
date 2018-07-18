#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

UNIFORM(1) vec4 color0;
UNIFORM(2) vec4 color1;
UNIFORM(3) float color_phase;
UNIFORM(4) float color_freq;
UNIFORM(5) float alphamod;
UNIFORM(6) float length;

void main(void) {
    vec2 uv_scaled = (vec4(texCoordRaw,0.0,1.0)*r_textureMatrix).xy; // what the hell

    float edgefactor = 20.0 * pow(alphamod, 2.0);
    float edgemod = min(1.0, uv_scaled.x * edgefactor) * clamp((length - uv_scaled.x) * edgefactor, 0.0, 1.0);
    float a = alphamod * edgemod;

    vec4 color = mix(color0, color1, 0.5 + 0.5 * sin(color_phase + color_freq * uv_scaled.x));

    fragColor = texture(tex, texCoord);
    fragColor *= color * a;
}
