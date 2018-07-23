#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"
#include "lib/util.glslh"

UNIFORM(1) float t;
UNIFORM(2) vec2 plrpos;
UNIFORM(3) float decay;

void main(void) {
	// This thing is supposed to produce a swirly texture centered
	// around plrpos. To do it we have to map to polar coordinates

	vec2 x = texCoordRaw-plrpos;
	float r = length(x) - 2 * t * (1 + decay *t);
	float phi = atan(x.y, x.x) / pi * 4;

	// give the angle some juicy r dependent offset to twist the swirl.
	phi += (1 + r) * t * 5 * decay;
	
	vec4 texel = texture(tex, vec2(phi,r));
	
	float fade =  0.7 * min(1,t) * (1 - t * decay) * 4;
	fragColor = texel * r_color * fade;
}
