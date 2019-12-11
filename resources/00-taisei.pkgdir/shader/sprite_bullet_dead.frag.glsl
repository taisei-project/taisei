#version 330 core

#include "lib/sprite_main.frag.glslh"

void spriteMain(out vec4 fragColor) {
    vec4 texel = texture(tex, texCoord);
    float oWhite = texel.b * (1 - clamp(2 * (1 - customParams.r),     0, 1));
    float oColor = texel.g * (1 - clamp(2 * (1 - customParams.r) - 1, 0, 1));
    float o = clamp(oWhite + oColor, 0, 1);
    vec4 col = (texel.g * color + vec4(texel.b)) * o;
    col.a *= o;

    fragColor = col;
}
