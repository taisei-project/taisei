#version 330 core

#include "interface/standard.glslh"

UNIFORM(1) sampler2D alphamap;
UNIFORM(2) int have_alphamap;

void main(void) {
	fragColor = textureLod(tex, texCoordRaw, 0);
	fragColor.rgb *= fragColor.a;

	if(have_alphamap != 0) {
		fragColor.a *= textureLod(alphamap, texCoordRaw, 0).r;
	}
}
