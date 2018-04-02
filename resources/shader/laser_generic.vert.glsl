#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"

layout(location = 0) in vec2 position;
layout(location = 2) in vec2 texCoordRawIn;

layout(location = 3) in vec4 instance_pos_delta;

out vec2 texCoord;
out vec2 texCoordRaw;

uniform float timeshift;
uniform float width;
uniform float width_exponent;
uniform int span;

#include "lasers/vertex_pos.glslh"

void main(void) {
    vec2 pos     = laser_vertex_pos(instance_pos_delta.xy, instance_pos_delta.zw);
    gl_Position  = ctx.modelViewMatrix * ctx.projectionMatrix * vec4(pos, 0.0, 1.0);
    texCoord     = (ctx.textureMatrix * vec4(texCoordRawIn, 0.0, 1.0)).xy;
    texCoordRaw  = texCoordRawIn;
}
