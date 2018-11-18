#version 330 core

#include "lib/defs.glslh"
#include "lib/render_context.glslh"
#include "interface/standard.glslh"
#include "lib/util.glslh"

UNIFORM(1) float tbomb;
UNIFORM(2) vec2 myon;

void main(void) {
	vec2 pos = texCoord;

	vec2 relativePos = pos-myon;
	float radius = length(relativePos);
	float angle = atan(relativePos.y, relativePos.x);
	float envelope = tbomb*(1-tbomb);

	float bladeFac = mod(angle*3+10*radius*radius-200*tbomb,2*pi)/2/pi;
	float angleDistortion = bladeFac*exp(-radius*radius/envelope/envelope);
	angle += angleDistortion;
	pos = myon + radius*vec2(cos(angle), sin(angle));

	float bladeShine = pow(bladeFac,4)/(1+pow(radius/envelope,5));
	fragColor = texture(tex, pos)+vec4(0.5,0,1,0)*bladeShine;
}
