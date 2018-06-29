#version 330 core

#include "lib/util.glslh"
#include "interface/spellcard.glslh"

void main(void) {
	float n = 10.;
	vec2 pos = (vec2(texCoordRaw)-origin*vec2(ratio,1.0))*n;
	pos.x /= ratio;
	pos = vec2(atan(pos.x,pos.y),length(pos)+n*t*0.1);
	pos.x *= h/w;
	pos.x += t*0.5*float(2*int(mod(pos.y,2.0))-1);
	pos = mod(pos,vec2(1.0+(0.01/w),1.0));
	pos *= vec2(w,h);
	vec4 clr = texture(tex, pos);
	clr.gb = clr.rr;
	clr.a *= float(pos.x < w && pos.y < h)*(2.*t-t*t);

	fragColor = clr;
}
