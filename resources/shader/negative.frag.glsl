#version 330 core

#include "interface/standard.glslh"

void main(void) {
	vec4 texel = texture(tex, texCoord);
	vec4 mixclr = 1.0-texel;
	mixclr.a = texel.a;
	fragColor = mixclr;
}
