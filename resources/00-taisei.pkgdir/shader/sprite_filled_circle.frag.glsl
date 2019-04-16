#version 330 core

#include "lib/sprite_main.frag.glslh"

UNIFORM(1) vec4 color_inner;
UNIFORM(2) vec4 color_outer;

void spriteMain(out vec4 fragColor) {
	float dist = length(texCoordRaw - vec2(0.5, 0.5));

	if(dist > 0.5) {
		discard;
	}

	fragColor = color * mix(color_inner, color_outer, dist * 2.0);
}
