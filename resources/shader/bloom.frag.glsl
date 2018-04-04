#version 330 core

#include "lib/util.glslh"
#include "lib/render_context.glslh"
#include "interface/standard.glslh"

uniform(1) float intensity;
uniform(2) float radius;
uniform(3) int samples;

void main(void) {
	vec2 pos = vec2(texCoordRaw);
	vec4 sum = vec4(0.0);
	float afactor = pi * 2.0 / float(samples);

	for(int a = 0; a < samples; a++) {
		vec2 npos = pos + vec2(cos(afactor*float(a)), sin(afactor*float(a))) * radius;
		sum += texture(tex, (r_textureMatrix*vec4(clamp(npos,0.0,0.9),0.0,1.0)).xy);
	}
	sum /= float(samples);

	fragColor = texture(tex, texCoord) + intensity*sum*sum*10.0;
}
