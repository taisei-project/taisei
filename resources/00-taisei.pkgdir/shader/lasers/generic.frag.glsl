#version 330 core

#include "../lib/render_context.glslh"
#include "../interface/laser.glslh"

void main(void) {
	fragColor = r_color * texture(tex, texCoord);
}
