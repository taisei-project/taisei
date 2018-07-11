#version 330 core

#include "interface/sprite.glslh"

void main(void) {
    fragColor = color * vec4(texture(tex, texCoord).r);
}
