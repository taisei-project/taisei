#version 330 core

#include "interface/standard.glslh"

void main(void) {
	fragColor = textureLod(tex, texCoordRaw, 0);
	fragColor.rgb *= fragColor.a;
}
