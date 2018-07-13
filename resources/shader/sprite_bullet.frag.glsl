#version 330 core

#include "interface/sprite.glslh"

void main(void) {
    vec4 texel = texture(tex, texCoord);
    fragColor = texel.g * color + vec4(texel.b);
}
