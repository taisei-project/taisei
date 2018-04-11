#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

UNIFORM(1) float t;
UNIFORM(2) vec2 plrpos;
UNIFORM(3) float decay;

void main(void) {
	vec2 r = texCoordRaw-plrpos;
	r = vec2(atan(r.y,r.x)/3.1415*4.,length(r)-2.*t*(1.+decay*t));
	r.x+=(1.+r.y)*t*5.*decay;
	vec4 texel = texture(tex, r);
	texel.a = 0.7*min(1.,t)*(1.-t*decay)*4.;

	fragColor = texel*r_color;
}
