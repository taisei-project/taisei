#version 330 core

#include "lib/sprite_main.frag.glslh"

void spriteMain(out vec4 fragColor) {
	vec4 texel = texture(tex, texCoord);
	fragColor = color * vec4((1.0 - texel.rgb / max(0.01, texel.a)) * texel.a, 0);
}
