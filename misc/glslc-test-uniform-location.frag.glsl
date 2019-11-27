#version 330 core
#extension GL_ARB_explicit_uniform_location : require

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform vec4 color;

void main(void) {
    outColor = texture(tex, texCoord) * color;
}
