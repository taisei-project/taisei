#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"
#include "lib/util.glslh"

UNIFORM(1) float time;
UNIFORM(2) float strength;

float g(float x) {
	return sin(1.1 * x) * cos(1.98 * x);
}

float wave(float x) {
	return g(x - 3 * g(0.6 * x) + 0.23 * g(1.2123 * x) + 0.7634 * g(1.12365 * x) + 1.541 * g(0.26 * x));
}

float light_mask(vec2 tc) {
	float p = 20.0;

	tc.x = mix(
		clamp(0.5 - 0.5 * pow(1 - tc.x * 2, p),     0.0, 0.5),
		clamp(0.5 + 0.5 * pow((tc.x - 0.5) * 2, p), 0.5, 1.0),
		step(0.5, tc.x)
	);

	return smoothstep(1.0, 0.0, length((tc - 0.5) * 2.0));
}

void main(void) {
	float s = 0.6;
	float l = 1.0;
	float o = 0.123 * texCoord.x;

	vec3 layer0 = texture(tex, texCoord + vec2(0, 0.23134 * time)).r * hsv2rgb(vec3(time + (texCoord.y + 1*o), s, l));
	layer0 *= pow(0.5 + 0.5 * wave(3215*pi + 0.9314*texCoord.y - time * 0.46), 1.0);

	vec3 layer1 = texture(tex, texCoord + vec2(0, 0.32155 * time)).r * hsv2rgb(vec3(time - (texCoord.y + 2*o), s, l));
	layer1 *= pow(0.5 + 0.5 * wave(7234*pi + 0.9612*texCoord.y + time * 0.64), 1.0);

	vec3 layer2 = texture(tex, texCoord - vec2(0, 0.30133 * time)).r * hsv2rgb(vec3(time + (texCoord.y + 3*o), s, l));
	layer2 *= pow(0.5 + 0.5 * wave(4312*pi + 0.9195*texCoord.y + time * 0.42), 1.0);

	vec3 layer3 = texture(tex, texCoord - vec2(0, 0.26424 * time)).r * hsv2rgb(vec3(time - (texCoord.y + 4*o), s, l));
	layer3 *= pow(0.5 + 0.5 * wave(2642*pi + 0.9195*texCoord.y + time * 0.60), 1.0);

	float mask = light_mask(texCoord);
	fragColor = vec4(layer0 + layer1 + layer2 + layer3, 4) * mask * strength * (1.5 + 0.25 * sin(time));
	fragColor.a = 0;
}
