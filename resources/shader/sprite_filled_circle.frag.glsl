#version 330 core

#include "lib/render_context.glslh"
#include "interface/sprite.glslh"

UNIFORM(1) vec4 color_inner;
UNIFORM(2) vec4 color_outer;

void main(void) {
	float dist = length(texCoordRaw - vec2(0.5, 0.5));

	if(dist > 0.5) {
		discard;
	}

	fragColor = color * mix(color_inner, color_outer, dist * 2.0);
}
