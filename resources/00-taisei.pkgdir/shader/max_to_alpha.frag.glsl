#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

void main(void) {
	fragColor = texture(tex, texCoord);
	fragColor.a = max(fragColor.r, max(fragColor.g, fragColor.b));
}
