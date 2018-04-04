#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/standard.glslh"

attribute(3) vec4 instance_pos_delta;

uniform(1) float timeshift;
uniform(2) float width;
uniform(3) float width_exponent;
uniform(4) int span;

#include "lasers/vertex_pos.glslh"

void main(void) {
    vec2 pos     = laser_vertex_pos(instance_pos_delta.xy, instance_pos_delta.zw);
    gl_Position  = r_modelViewMatrix * r_projectionMatrix * vec4(pos, 0.0, 1.0);
    texCoord     = (r_textureMatrix * vec4(texCoordRawIn, 0.0, 1.0)).xy;
    texCoordRaw  = texCoordRawIn;
}
