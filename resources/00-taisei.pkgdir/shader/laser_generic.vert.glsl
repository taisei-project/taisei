#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/standard.glslh"

ATTRIBUTE(3) vec4 instance_pos_delta;
ATTRIBUTE(4) float instance_width;

UNIFORM(1) float timeshift;
UNIFORM(2) float width;
UNIFORM(3) float width_exponent;
UNIFORM(4) int span;

#include "lasers/vertex_pos.glslh"

void main(void) {
    vec2 pos     = laser_vertex_pos(instance_pos_delta.xy, instance_pos_delta.zw, instance_width);
    gl_Position  = r_modelViewMatrix * r_projectionMatrix * vec4(pos, 0.0, 1.0);
    texCoord     = (r_textureMatrix * vec4(texCoordRawIn, 0.0, 1.0)).xy;
    texCoordRaw  = texCoordRawIn;
}
