#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/laser.glslh"

#include "lasers/vertex_pos.glslh"

void main(void) {
    vec2 pos     = laser_vertex_pos(instance_pos_delta.xy, instance_pos_delta.zw, instance_width);
    gl_Position  = r_projectionMatrix * r_modelViewMatrix * vec4(pos, 0.0, 1.0);
    texCoord     = (r_textureMatrix * vec4(texCoordRawIn, 0.0, 1.0)).xy;
    texCoordRaw  = texCoordRawIn;
}
