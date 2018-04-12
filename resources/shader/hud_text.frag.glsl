#version 330 core

#include "interface/standard.glslh"

UNIFORM(1) vec4 colorAtop;
UNIFORM(2) vec4 colorAbot;
UNIFORM(3) vec4 colorBtop;
UNIFORM(4) vec4 colorBbot;
UNIFORM(5) vec4 colortint;
UNIFORM(6) float split;

void main(void) {
	vec4 texel = texture(tex, texCoord);

	float vsplit;
	vec4 cAt;
	vec4 cAb;
	vec4 cBt;
	vec4 cBb;

	// branching on a uniform is fine
	if(split < 0.0) {
		vsplit = -split;
		cAt = colorBtop;
		cAb = colorBbot;
		cBt = colorAtop;
		cBb = colorAbot;
	} else {
		vsplit =  split;
		cAt = colorAtop;
		cAb = colorAbot;
		cBt = colorBtop;
		cBb = colorBbot;
	}

	fragColor = texel * colortint * (
		texCoord.x >= vsplit ?
			mix(cBt, cBb, texCoord.y) :
			mix(cAt, cAb, texCoord.y)
	);
}
