#version 330 core

#include "lib/util.glslh"
#include "interface/spellcard.glslh"

void main(void) {
	vec2 pos = texCoordRaw;
	vec4 clr = texture(tex, texCoord);
	pos -= origin;
	pos.y *= ratio;
	float r = length(pos);
	float phi = atan(pos.y,pos.x)+t/10.0;

	// everything bigger than rmax is not drawn. The sine creates the
	// spinning wavy border.
	float rmax = 1 + _smoothstep(t - 0.5) * sin(t * 40 + 10 * phi);

	// now make it grow with time.
	rmax *= step(0, t) * t * 1.5;

	// at the rim, invert colors
	fragColor = mix(clr, vec4(1) - vec4(clr.rgb, 0), step(rmax-0.1*sqrt(r),r));

	fragColor *= step(r, rmax);
}
