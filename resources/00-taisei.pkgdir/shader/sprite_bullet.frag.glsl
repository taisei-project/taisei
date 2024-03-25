#version 330 core

#include "lib/sprite_main.frag.glslh"

#include "lib/util.glslh"

void spriteMain(out vec4 fragColor) {
	vec4 mask = texture(tex, texCoord);

	if(mask.a == 0.0) {
		discard;
	}

	vec4 color2 = smoothstep(mask.g - 0.25, 1 + 0.5 * mask.r, 0.1 + 0.8 * color);
	fragColor = (color * mask.g + mix(mask.r * color2, vec4(1.0), mask.b)) * customParams.r;
}
