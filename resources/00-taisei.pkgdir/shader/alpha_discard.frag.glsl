#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

UNIFORM(1) float threshold;

void main(void) {
	vec4 clr = texture(tex, texCoord);

	if(clr.a < threshold) {
		discard;
	}

	fragColor = clr;
}
