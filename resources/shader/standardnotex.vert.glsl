#version 330 core

#include "lib/render_context.glslh"

layout(location=0) in vec3 position;

void main(void) {
    gl_Position = ctx.projectionMatrix * ctx.modelViewMatrix * vec4(position, 1.0);
}
