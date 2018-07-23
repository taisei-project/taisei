#version 330 core

#include "interface/sprite.glslh"

void main(void) {
	vec4 texel = texture(tex, texCoord);
	fragColor = vec4((1.0 - texel.rgb / max(0.01, texel.a)) * texel.a, 0);
}
