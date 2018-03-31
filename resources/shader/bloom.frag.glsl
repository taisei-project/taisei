#version 330 core

#include "lib/render_context.glslh"

uniform sampler2D tex;
uniform float intensity;
uniform float radius;
uniform int samples;

in vec2 texCoord;
in vec2 texCoordRaw;
out vec4 fragColor;

#define PI 3.1415926535897932384626433832795

void main(void) {
	vec2 pos = vec2(texCoordRaw);
	vec4 sum = vec4(0.0);
	float afactor = PI * 2.0 / float(samples);

	for(int a = 0; a < samples; a++) {
		vec2 npos = pos + vec2(cos(afactor*float(a)), sin(afactor*float(a))) * radius;
		sum += texture(tex, (ctx.textureMatrix*vec4(clamp(npos,0.0,0.9),0.0,1.0)).xy);
	}
	sum /= float(samples);

	fragColor = texture(tex, texCoord) + intensity*sum*sum*10.0;
}
