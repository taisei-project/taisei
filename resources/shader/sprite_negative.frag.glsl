#version 330 core

#include "interface/sprite.glslh"

void main(void) {
	vec4 texel = texture(tex, texCoord);
	vec4 mixclr = 1.0-texel;
	// mixclr.a = texel.a;
	mixclr.a = 0;
	fragColor = mixclr;
}
