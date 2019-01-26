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
	
	// everything lesser than rmin is not drawn. The sine creates the
	// spinning wavy border.
	float rmin = (1 + 0.3 * sin(t * 20 + 10 * phi));

	// now grow it
	rmin *= step(0.,t) * t * t * 10;

	// at the rim, invert colors
	fragColor = mix(clr, vec4(1.0) - vec4(clr.rgb,0.), step(r, rmin + t * (0.1 + 0.1 * sin(10 * r))));

	fragColor *= step(rmin, r);
}
