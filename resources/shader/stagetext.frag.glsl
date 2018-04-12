#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

UNIFORM(1) sampler2D trans;
UNIFORM(2) vec3 color;
UNIFORM(3) float t;

void main(void) {
	vec2 pos = texCoordRaw;
	vec2 f = pos-vec2(0.5,0.5);
	pos += (f*0.05*sin(10.0*(length(f)+t)))*(1.0-t);

	pos = clamp(pos,0.0,1.0);
	vec4 texel = texture(tex, (r_textureMatrix*vec4(pos,0.0,1.0)).xy);

	texel.a *= clamp((texture(trans, pos).r+0.5)*2.5*t-0.5, 0.0, 1.0);
	fragColor = vec4(color,1.0)*texel;
}

