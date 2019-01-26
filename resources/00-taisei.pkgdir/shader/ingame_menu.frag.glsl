#version 330 core

#include "lib/util.glslh"
#include "interface/standard.glslh"

UNIFORM(1) float rad;
UNIFORM(2) float phase;

void main(void) {
	vec2 pos = texCoord;

	fragColor = texture(tex, pos)*0.02;

	float a, r = rad*0.06;
	for(a = 0.0; a < 2.0*pi; a += pi/5.0)
		fragColor += texture(tex, pos + r*vec2(cos(phase+a*dot(pos, vec2(10.0,3.0))),sin(phase+a*a)))*0.1;

	fragColor *= sqrt(fragColor)*pow(vec4(0.4,0.5,0.6,1), vec4(rad*25.0));
}

