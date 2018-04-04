#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

uniform(1) vec4 color0;
uniform(2) vec4 color1;
uniform(3) float color_phase;
uniform(4) float color_freq;
uniform(5) float alphamod;
uniform(6) float length;

void main(void) {
    vec2 uv_scaled = (vec4(texCoordRaw,0.0,1.0)*r_textureMatrix).xy; // what the hell

    float edgefactor = 20.0 * pow(alphamod, 2.0);
    float edgemod = min(1.0, uv_scaled.x * edgefactor) * clamp((length - uv_scaled.x) * edgefactor, 0.0, 1.0);
    float a = alphamod * edgemod;

    vec4 color = mix(color0, color1, 0.5 + 0.5 * sin(color_phase + color_freq * uv_scaled.x));

    fragColor = texture(tex, texCoord);
    fragColor *= vec4(color.rgb, color.a * a);
}
