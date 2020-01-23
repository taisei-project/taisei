#version 330 core

#define BLUR9_SAMPLER_FUNC(s, uv) max(vec4(0), texture(s, uv))

#include "interface/standard.glslh"
#include "lib/blur/blur9.glslh"
#include "lib/util.glslh"

UNIFORM(1) vec2 blur_resolution;
UNIFORM(2) vec2 blur_direction;
UNIFORM(3) float fade;

#define ONE_OVER_LN2 1.4426950408889634

float dim(float x) {
	return max(0, smoothmin(x, 3, 1));
}

vec3 dim(vec3 v) {
	return vec3(dim(v.x), dim(v.y), dim(v.z));
}

void main(void) {
	vec3 c = sample_blur9(tex, texCoord, blur_direction / blur_resolution).rgb;
	// c = mix(c, smoothstep(0.0, 1.0, c), sqrt(fade));
	// c = alphaCompose(c, vec4(0, 0, 0, 0.1*fade));

	#if 0
	if(fade != 1.0) {
		c = dim(c) * fade;
	}
	#else
	c *= fade;
	#endif

	fragColor = vec4(c, 0);
}
