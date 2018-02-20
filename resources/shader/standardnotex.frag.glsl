#version 330 core

layout(std140) uniform RenderContext {
    mat4 modelViewMatrix;
    mat4 projectionMatrix;
    mat4 textureMatrix;
    vec4 color;
} ctx;

out vec4 fragColor;

void main(void) {
    fragColor = ctx.color;
}
