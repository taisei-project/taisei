#version 330 core

#include "render_context.glslh"

uniform sampler2D tex;

in vec2 texCoord;
in vec4 color;
in float customParam;

out vec4 fragColor;

/*
ported from:

.R[1] = mix_colors(c_b, c_r, sqrt(charge)),
.G[1] = c_g,
.B[1] = mix_colors(multiply_colors(c_r, rgba(2, 0, 0, 0)), c_b, 0.75 * charge),
.A[1] = c_a,
*/

void main(void) {
    vec4 texel = texture(tex, texCoord);
    float charge = customParam;

    fragColor = vec4(0.0);
    fragColor.rgb += texel.r * mix(vec3(color.r, 0.0, 0.0), vec3(0.0, 0.0, color.b), 0.75 * sqrt(charge));
    fragColor.g += texel.g * color.g;
    fragColor.rgb += texel.b * mix(vec3(0.0, 0.0, color.b), vec3(2.0 * color.r, 0.0, 0.0), 0.75 * charge);
    fragColor.a = texel.a * color.a;
}
