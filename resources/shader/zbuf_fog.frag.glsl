#version 330 core

#include "lib/defs.glslh"
#include "interface/standard.glslh"

UNIFORM(1) sampler2D depth;
UNIFORM(2) float start;
UNIFORM(3) float end;
UNIFORM(4) float exponent;
UNIFORM(5) float sphereness;
UNIFORM(6) vec4 fog_color;

void main(void) {
	vec2 pos = vec2(texCoord);

	float z = pow(texture(depth, texCoord).x+sphereness*length(texCoordRaw-vec2(0.5,0.0)), exponent);
	float f = clamp((end - z)/(end-start),0.0,1.0);

	fragColor = f*texture(tex, texCoord) + (1.0-f)*fog_color;
}
