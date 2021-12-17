
#version 330 core

#include "lib/pbr.glslh"
#include "interface/standard.glslh"

UNIFORM(1) vec3 exposure;

void main(void) {
	vec4 c = texture(tex, texCoord);
	c.rgb = PBR_TonemapDefault(exposure * c.rgb);
	c.rgb = PBR_GammaCorrect(c.rgb);
	fragColor = c;
}
