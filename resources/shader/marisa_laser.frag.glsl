#version 330 core

#include "lib/render_context.glslh"

uniform sampler2D tex;
uniform vec4 color0;
uniform vec4 color1;
uniform float color_phase;
uniform float color_freq;
uniform float alphamod;
uniform float length;

in vec2 texCoord;
in vec2 texCoordRaw;
out vec4 fragColor;

void main(void) {
    vec2 uv_scaled = (vec4(texCoordRaw,0.0,1.0)*ctx.textureMatrix).xy; // what the hell

    float edgefactor = 20.0 * pow(alphamod, 2.0);
    float edgemod = min(1.0, uv_scaled.x * edgefactor) * clamp((length - uv_scaled.x) * edgefactor, 0.0, 1.0);
    float a = alphamod * edgemod;

    vec4 color = mix(color0, color1, 0.5 + 0.5 * sin(color_phase + color_freq * uv_scaled.x));

    fragColor = texture2D(tex, texCoord);
    fragColor *= vec4(color.rgb, color.a * a);
}
