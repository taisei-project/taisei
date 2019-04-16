#version 330 core

#include "lib/sprite_main.frag.glslh"

void spriteMain(out vec4 fragColor) {
    fragColor = color * vec4(texture(tex, texCoord).r);
}
