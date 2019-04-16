#version 330 core

#include "lib/sprite_main.frag.glslh"

void spriteMain(out vec4 fragColor) {
    vec4 texel = texture(tex, texCoord);
    fragColor = (texel.g * color + vec4(texel.b)) * (1 - customParams.r);
}
