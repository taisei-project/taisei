#version 330 core

#include "lib/sprite_main.frag.glslh"

/*
ported from:

.R[1] = mix_colors(c_b, c_r, sqrt(charge)),
.G[1] = c_g,
.B[1] = mix_colors(multiply_colors(c_r, rgba(2, 0, 0, 0)), c_b, 0.75 * charge),
.A[1] = c_a,
*/

void spriteMain(out vec4 fragColor) {
    vec4 texel = texture(tex, texCoord);
    float charge = customParams.r;

    fragColor = vec4(0.0);
    fragColor.rgb += texel.r * mix(vec3(color.r, 0.0, 0.0), vec3(0.0, 0.0, color.b), sqrt(charge));
    fragColor.g += texel.g * color.g;
    fragColor.rgb += texel.b * mix(vec3(0.0, 0.0, color.b), vec3(2.0 * color.r, 0.0, 0.0), 0.75 * charge);
    fragColor.a = texel.a * color.a;
    fragColor *= customParams.g;
}
