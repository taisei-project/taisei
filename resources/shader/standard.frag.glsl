#version 330 core

#include "lib/render_context.glslh"

uniform sampler2D tex;

in vec2 texCoord;
in vec2 texCoordRaw;
out vec4 fragColor;

void main(void) {
	fragColor = ctx.color * texture(tex, texCoord);
}
