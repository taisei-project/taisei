#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

void main(void) {
	fragColor = r_color * texture(tex, texCoord);
}
