#version 330 core

uniform vec4 colorAtop;
uniform vec4 colorAbot;
uniform vec4 colorBtop;
uniform vec4 colorBbot;
uniform vec4 colortint;
uniform float split;

uniform sampler2D tex;
in vec2 texCoord;
in vec2 texCoordRaw;
out vec4 fragColor;

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
