#version 330 core

#include "lib/fxaa.glslh"
#include "interface/standard.glslh"
#include "lib/render_context.glslh"

VARYING(3) vec4 fxaa_tc;

void main(void) {
    gl_Position = r_projectionMatrix * r_modelViewMatrix * vec4(position, 1.0);
    fxaa_tc = fxaaCoords(tex, texCoordRawIn);
}
