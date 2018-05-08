#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

void main(void) {
    gl_Position = r_projectionMatrix * r_modelViewMatrix * vec4(position, 1.0);
    texCoord = texCoordRaw = texCoordRawIn;
}
