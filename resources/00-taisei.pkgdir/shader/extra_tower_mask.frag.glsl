#version 330 core

#include "interface/tower_light.glslh"
#include "lib/util.glslh"

UNIFORM(24) float time;
UNIFORM(25) sampler2D tex2;
UNIFORM(26) float dissolve;

void main(void) {
	vec4 texel2 = texture(tex2, texCoord);
	vec4 texel = texture(tex, texCoord);

	float m = texel.r;
	m = 0.5 + 0.5 * sin(pi*m + time);
	m = 0.5 + 0.5 * sin(2*pi*m + time);

	float dissolve_phase = smoothstep(dissolve * dissolve, dissolve, texel.r);
	float d = mix(1.0 - texel2.r, 1.0, dissolve_phase);

	fragColor = vec4(m, d, texel2.g, texel.r);
}
