#version 330 core

#include "lib/render_context.glslh"

uniform sampler2D tex;

in vec2 texCoordRaw;
in vec2 texCoord;
in vec4 color;
in float customParam;

out vec4 fragColor;

void main(void) {
    fragColor = color * texture(tex, texCoord);
}
