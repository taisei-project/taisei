#version 330 core

#include "lib/sprite_main.frag.glslh"

void spriteMain(out vec4 fragColor) {
	vec4 texel = texture(tex, texCoord);

	if(texel.a == 0.0) {
		discard;
	}

	fragColor = (color * texel.g + vec4(texel.b)) * customParams.r;
}
