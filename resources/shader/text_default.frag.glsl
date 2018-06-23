#version 330 core

#include "interface/sprite.glslh"

void main(void) {
    fragColor = color * vec4(1, 1, 1, texture(tex, texCoord).r);
}
