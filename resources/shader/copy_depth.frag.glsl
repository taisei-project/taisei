#version 330 core

#include "interface/standard.glslh"

void main(void) {
    gl_FragDepth = texture(tex, texCoord).r;
}
