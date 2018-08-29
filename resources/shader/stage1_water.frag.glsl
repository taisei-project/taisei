#version 330 core

#include "lib/util.glslh"
#include "interface/standard.glslh"

UNIFORM(1) float time;

// Based on https://www.shadertoy.com/view/Xl2XWz

float smoothNoise(vec2 p) {
	vec2 f = fract(p);
	p -= f;
	f *= f * (3 - f - f);

	return dot(mat2(fract(sin(vec4(0, 1, 27, 28) + p.x + p.y * 27) * 1e5)) * vec2(1 - f.y, f.y), vec2(1 - f.x, f.x));
}

float fractalNoise(vec2 p) {
	return
		smoothNoise(p)     * 0.5333 +
		smoothNoise(p * 2) * 0.2667 +
		smoothNoise(p * 4) * 0.1333 +
		smoothNoise(p * 8) * 0.0667;
}

float warpedNoise(vec2 p) {
	vec2 m = vec2(0.0, -time);
	float x = fractalNoise(p + m);
	float y = fractalNoise(p + m.yx + x);
	float z = fractalNoise(p - m - x);
	return fractalNoise(p + vec2(x, y) + vec2(y, z) + vec2(z, x) + length(vec3(x, y, z)) * 0.1);
}

void main(void) {
	vec2 uv = flip_native_to_bottomleft(texCoord - vec2(0, time * 0.2)) * rot(-pi/4);

	float n = warpedNoise(uv * 4);
	float n2 = warpedNoise(uv * 4 + 0.005);

	float bump  = max(n2 - n, 0) / 0.02 * 0.7071;
	float bump2 = max(n - n2, 0) / 0.02 * 0.7071;

	bump  = bump  * bump  * 0.5 + pow(bump,  4) * 0.5;
	bump2 = bump2 * bump2 * 0.5 + pow(bump2, 4) * 0.5;

	vec4 cmod = vec4(0.5, 0.8, 0.8, 1.0);

	uv = flip_native_to_bottomleft(texCoord);
	uv += 0.25 * vec2(bump, bump2);
	uv = flip_bottomleft_to_native(uv);

	fragColor = mix(cmod * texture(tex, uv), vec4(0.8, 0.8, 1.0, 1.0), (sqrt(bump2) - sqrt(bump) * 0.25) * 0.05);
}
