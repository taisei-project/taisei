#version 330 core

#include "lib/render_context.glslh"

out vec4 fragColor;

void main(void) {
    fragColor = ctx.color;
}
